#include "Console.h"

#include "../src/serv/SessionPool.h"
#include "SystemMgr.h"

#include <setjmp.h> // NOLINT(hicpp-deprecated-headers,modernize-deprecated-headers)

#include <algorithm>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <readline/history.h> // NOLINT(build/include_order): cpplint considers readline headers as C system headers.
#include <readline/readline.h> // NOLINT(build/include_order)

#include "Pagination.h"

#define ANSI_COLOR_RED "\x1B[31m"   // NOLINT(cppcoreguidelines-macro-usage)
#define ANSI_COLOR_GREEN "\x1B[32m" // NOLINT(cppcoreguidelines-macro-usage)
#define ANSI_COLOR_RESET "\x1B[0m"  // NOLINT(cppcoreguidelines-macro-usage)

#define ANSI_COLOR_RED_RL                                                      \
  "\001\x1B[31m\002" // NOLINT(cppcoreguidelines-macro-usage)
#define ANSI_COLOR_GREEN_RL                                                    \
  "\001\x1B[32m\002" // NOLINT(cppcoreguidelines-macro-usage)
#define ANSI_COLOR_RESET_RL                                                    \
  "\001\x1B[0m\002" // NOLINT(cppcoreguidelines-macro-usage)

namespace {

/**
 * Buffer for program state
 *
 * We use this to make Ctrl+C work on all platforms by jumping back into main()
 * from the Ctrl+C signal handler. This was the only way to get this to work on
 * all platforms inclusing macOS. See here
 * (https://github.com/hyrise/hyrise/pull/198#discussion_r135539719) for a
 * discussion about this.
 *
 * The known caveats of goto/longjmp aside, this will probably also cause
 * problems (queries continuing to run in the background) when the
 * scheduler/multithreading is enabled.
 */
sigjmp_buf
    jmp_env; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// Returns a string containing a timestamp of the current date and time.
std::string current_timestamp() {
  auto time = std::time(nullptr);
  const auto local_time = *std::localtime(
      &time); // NOLINT(concurrency-mt-unsafe): not called concurrently.

  auto oss = std::ostringstream{};
  oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

// Removes the coloring commands (e.g. '\x1B[31m') from input, to have a clean
// logfile. If remove_rl_codes_only is true, then it only removes the Readline
// specific escape sequences '\001' and '\002'
std::string remove_coloring(const std::string &input,
                            bool remove_rl_codes_only = false) {
  // Matches any characters that need to be escaped in RegEx except for '|'.
  const auto special_chars = std::regex{R"([-[\]{}()*+?.,\^$#\s])"};
  auto sequences = std::string{"\x1B[31m|\x1B[32m|\x1B[0m|\001|\002"};
  if (remove_rl_codes_only) {
    sequences = "\001|\002";
  }
  const auto sanitized_sequences =
      std::regex_replace(sequences, special_chars, R"(\$&)");

  // Remove coloring commands and escape sequences before writing to logfile.
  const auto expression = std::regex{"(" + sanitized_sequences + ")"};
  return std::regex_replace(input, expression, "");
}

std::vector<std::string> tokenize(std::string input) {
  boost::algorithm::trim<std::string>(input);

  // Remove whitespace duplicates to not get empty tokens after
  // boost::algorithm::split.
  const auto both_are_spaces = [](char left, char right) {
    return (left == right) && (left == ' ');
  };
  input.erase(std::unique(input.begin(), input.end(), both_are_spaces),
              input.end());

  auto tokens = std::vector<std::string>{};
  boost::algorithm::split(tokens, input, boost::is_space());

  return tokens;
}

} // namespace

namespace storage {

Console::Console()
    : _prompt("> "), _out(std::cout.rdbuf()),
      _log("console.log", std::ios_base::app | std::ios_base::out),
      _verbose(false), _pagination_active(false) {
  // Init readline basics, tells readline to use our custom command completion
  // function.
  rl_attempted_completion_function = &Console::_command_completion;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  rl_completer_word_break_characters =
      const_cast<char *>(" \t\n\"\\'`@$><=;|&{(");

  // Register default commands.
  register_command("exit",
                   std::bind(&Console::_exit, this, std::placeholders::_1));
  register_command("quit",
                   std::bind(&Console::_exit, this, std::placeholders::_1));
  register_command("help",
                   std::bind(&Console::_help, this, std::placeholders::_1));

  register_command("pwd",
                   std::bind(&Console::_print_current_working_directory, this));
}

Console::~Console() {
  out("Bye.\n");

  // Timestamp dump only to logfile.
  out("--- Session end --- " + current_timestamp() + "\n", false);
}

int Console::read() {
  char *buffer = nullptr;

  // Prompt user for input.
  buffer = readline(_prompt.c_str());
  if (!buffer) {
    return ReturnCode::Quit;
  }

  auto input = std::string{buffer};
  boost::algorithm::trim<std::string>(input);

  // Only save non-empty commands to history.
  if (!input.empty()) {
    add_history(buffer);
    // Save command to history file.
    if (!_history_file.empty()) {
      if (append_history(1, _history_file.c_str()) != 0) {
        out("Error appending to history file: " + _history_file + "\n");
      }
    }
  }

  // Free buffer, since readline() allocates new string every time.
  std::free(
      buffer); // NOLINT(cppcoreguidelines-no-malloc,hicpp-no-malloc,cppcoreguidelines-owning-memory)

  return _eval(input);
}

int Console::_eval(const std::string &input) {
  // Do nothing if no input was given.
  if (input.empty() && _multiline_input.empty()) {
    return ReturnCode::Ok;
  }

  // Dump command to logfile, and to the Console if input comes from a script
  // file. Also remove Readline specific escape sequences ('\001' and '\002') to
  // make it look normal.
  out(remove_coloring(_prompt + input + "\n", true), _verbose);

  // Check if we already are in multiline input.
  if (_multiline_input.empty()) {
    // Check if a registered command was entered.
    const auto it =
        _commands.find(input.substr(0, input.find_first_of(" \n;")));
    if (it != _commands.end()) {
      return _eval_command(it->second, input);
    }

    // Regard query as complete if input is valid and not already in multiline.
    // auto parse_result = hsql::SQLParserResult{};
    // hsql::SQLParser::parse(input, &parse_result);
    // if (parse_result.isValid()) {
    //   return _eval_sql(input);
    // }
  }

  // Regard query as complete if last character is semicolon, regardless of
  // multiline or not.
  if (input.back() == ';') {
    const auto return_code = _eval_sql(_multiline_input + input);
    _multiline_input = "";
    return return_code;
  }

  // If query is not complete(/valid), and the last character is not a
  // semicolon, enter/continue multiline.
  _multiline_input += input;
  _multiline_input += '\n';
  return ReturnCode::Multiline;
}

int Console::_eval_command(const CommandFunction &func,
                           const std::string &command) {
  auto cmd = command;
  if (command.back() == ';') {
    cmd = command.substr(0, command.size() - 1);
  }
  boost::algorithm::trim<std::string>(cmd);

  const auto first = cmd.find(' ');
  const auto last = cmd.find('\n');

  // If no whitespace is found, zero arguments are provided.
  if (std::string::npos == first) {
    return static_cast<int>(func(""));
  }

  auto args = cmd.substr(first + 1, last - (first + 1));

  // Remove whitespace duplicates in args.
  const auto both_are_spaces = [](char left, char right) {
    return (left == right) && (left == ' ');
  };
  args.erase(std::unique(args.begin(), args.end(), both_are_spaces),
             args.end());

  return static_cast<int>(func(args));
}

int Console::_eval_sql(const std::string &sql) {
  SessionPool::AddStatement(0, 0, _stmtId++, _exprId++, sql.c_str(),
                            VectorRow(), &_stmtRes);
  while (_stmtRes.GetResultStatus() == ResultStatus::FILLING) {
    this_thread::yield();
  }

  if (_stmtRes._vctError.size() > 0) {
    for (MString &err : _stmtRes._vctError) {
      out(err.c_str());
      out("\n");
    }

    return ReturnCode::Error;
  }

  if (_stmtRes._resultSet != nullptr) {
    MVectorPtr<ExprColumn *> *vctCol = _stmtRes._resultSet->GetVctColumn();
    stringstream ss;
    for (ExprColumn *col : *vctCol) {
      ss << col->_name->c_str() << "\t";
    }

    out(ss.str());
    out("\n");
    bool b = _stmtRes._resultSet->First();
    while (b) {
      stringstream ssdata;
      VectorDataValue vdv;
      _stmtRes._resultSet->GetCurrDataValueRow(vdv);

      for (IDataValue *dv : vdv) {
        StrBuff sbuff(256);
        dv->ToString(sbuff);
        ssdata << sbuff.GetBuff() << "\t";
      }

      out(ssdata.str());
      out("\n");

      b = _stmtRes._resultSet->Next();
    }

    out(to_string(_stmtRes._resultSet->GetRowCount()) + " rows in set.");
    out("\n");
  } else {
    out("Query OK, " + to_string(_stmtRes._rowNum) + " rows affected.");
    out("\n");
  }

  return ReturnCode::Ok;
}

void Console::register_command(const std::string &name,
                               const CommandFunction &func) {
  _commands[name] = func;
}

void Console::set_prompt(const std::string &prompt) {
#ifdef _DEBUG
  _prompt = ANSI_COLOR_RED_RL "(debug)" ANSI_COLOR_RESET_RL + prompt;
#else
  _prompt = ANSI_COLOR_GREEN_RL "(release)" ANSI_COLOR_RESET_RL + prompt;
#endif
}

void Console::set_logfile(const std::string &logfile) {
  _log = std::ofstream(logfile, std::ios_base::app | std::ios_base::out);
}

void Console::set_console_path(const std::string &path) { _path = path; }

void Console::load_history(const std::string &history_file) {
  _history_file = history_file;

  // Check if history file exist, create empty history file if not.
  const auto file = std::ifstream{_history_file};
  if (!file.good()) {
    out("Creating history file: " + _history_file + "\n");
    if (write_history(_history_file.c_str()) != 0) {
      out("Error creating history file: " + _history_file + "\n");
      return;
    }
  }

  if (read_history(_history_file.c_str()) != 0) {
    out("Error reading history file: " + _history_file + "\n");
  }
}

void Console::out(const std::string &output, bool console_print) {
  if (console_print) {
    _out << output;
  }
  // Remove coloring commands like '\x1B[32m' when writing to logfile.
  _log << remove_coloring(output);
  _log.flush();
}

// Command functions

// NOLINTNEXTLINE: while this particular method could be made static, others
// cannot.
int Console::_exit(const std::string & /*args*/) { return ReturnCode::Quit; }

int Console::_help(const std::string & /*args*/) {
  auto encoding_options = std::string{
      "                                                 Encoding options: "};
  // Split the encoding options in lines of 120 and add padding. For each input
  // line, it takes up to 120 characters and replaces the following space(s)
  // with a new line. `(?: +|$)` is a non-capturing group that matches either a
  // non-zero number of spaces or the end of the line.
  const auto line_wrap = std::regex{"(.{1,120})(?: +|$)"};
  encoding_options =
      regex_replace(encoding_options, line_wrap,
                    "$1\n                                                    ");
  // Remove the 49 spaces and the new line added at the end.
  encoding_options.resize(encoding_options.size() - 50);

  // clang-format off
  out("HYRISE SQL Interface\n\n");
  out("Available commands:\n");
  out("  generate_tpcc NUM_WAREHOUSES [CHUNK_SIZE] - Generate all TPC-C tables\n");
  out("  generate_tpch SCALE_FACTOR [CHUNK_SIZE]   - Generate all TPC-H tables\n");
  out("  generate_tpcds SCALE_FACTOR [CHUNK_SIZE]  - Generate all TPC-DS tables\n");
  out("  generate_ssb SCALE_FACTOR [CHUNK_SIZE]    - Generate all SSB tables\n");
  out("  load FILEPATH [TABLENAME [ENCODING]]      - Load table from disk specified by filepath FILEPATH, store it with name TABLENAME\n");  // NOLINT(whitespace/line_length)
  out("                                                   The import type is chosen by the type of FILEPATH.\n");
  out("                                                     Supported types: '.bin', '.csv', '.tbl'\n");
  out("                                                   If no table name is specified, the filename without extension is used\n");  // NOLINT(whitespace/line_length)
  out(encoding_options + "\n");
  out("  export TABLENAME FILEPATH                 - Export table named TABLENAME from storage manager to filepath FILEPATH\n");  // NOLINT(whitespace/line_length)
  out("                                                 The export type is chosen by the type of FILEPATH.\n");
  out("                                                   Supported types: '.bin', '.csv'\n");
  out("  script SCRIPTFILE                         - Execute script specified by SCRIPTFILE\n");
  out("  print TABLENAME                           - Fully print the given table (including MVCC data)\n");
  out("  visualize [options] [SQL]                 - Visualize a SQL query\n");
  out("                                                 Options\n");
  out("                                                  - {exec, noexec} Execute the query before visualization.\n");
  out("                                                                   Default: exec\n");
  out("                                                  - {lqp, unoptlqp, pqp, joins} Type of plan to visualize. unoptlqp gives the\n");  // NOLINT(whitespace/line_length)
  out("                                                                         unoptimized lqp; joins visualized the join graph.\n");  // NOLINT(whitespace/line_length)
  out("                                                                         Default: pqp\n");
  out("                                                SQL\n");
  out("                                                  - Optional, a query to visualize. If not specified, the last\n");  // NOLINT(whitespace/line_length)
  out("                                                    previously executed query is visualized.\n");
  out("  txinfo                                    - Print information on the current transaction\n");
  out("  pwd                                       - Print current working directory\n");
  out("  load_plugin FILE                          - Load and start plugin stored at FILE\n");
  out("  unload_plugin NAME                        - Stop and unload the plugin libNAME.so/dylib (also clears the query cache)\n");  // NOLINT(whitespace/line_length)
  out("  quit                                      - Exit the HYRISE Console\n");
  out("  help                                      - Show this message\n");
  out("  setting [property] [value]                - Change a runtime setting\n");
  out("           scheduler (on|off)               - Turn the scheduler on (default) or off\n");
  out("           binary_caching (on|off)          - Use cached binary tables for benchmarks (default) or not\n");
  out("  reset                                     - Clear all stored tables and cached query plans and restore the default settings\n\n");  // NOLINT(whitespace/line_length)
  // clang-format on

  return ReturnCode::Ok;
}

void Console::handle_signal(int sig) {
  if (sig == SIGINT) {
    auto &console = Console::get();
    // When in pagination mode, just quit pagination. Otherwise, reset console.
    if (console._pagination_active) {
      Pagination::push_ctrl_c();
    } else {
      // Reset console state
      console._out << "\n";
      console._multiline_input = "";
      console.set_prompt("!> ");
      console._verbose = false;
      // Restore program state stored in jmp_env set with sigsetjmp(2). See
      // comment on jmp_env for details.
      siglongjmp(jmp_env, 1);
    }
  }
}

int Console::_print_current_working_directory() {
  out(std::filesystem::current_path().string() + "\n");
  return ReturnCode::Ok;
}

// GNU readline interface to our commands

char **Console::_command_completion(const char *text, const int start,
                                    const int /*end*/) {
  char **completion_matches = nullptr;

  const auto input = std::string{rl_line_buffer};

  const auto tokens = tokenize(input);

  // Choose completion function depending on the input.
  const auto &first_word = tokens[0];
  if (first_word == "quit" || first_word == "exit" || first_word == "help") {
    // Turn off filepath completion.
    rl_attempted_completion_over = 1;
  } else if (start == 0) {
    completion_matches =
        rl_completion_matches(text, &Console::_command_generator_default);
  }

  return completion_matches;
}

char *Console::_command_generator(const char *text, int state,
                                  const std::vector<std::string> &commands) {
  static std::vector<std::string>::const_iterator it;
  if (state == 0) {
    it = commands.begin();
  }

  for (; it != commands.end(); ++it) {
    const auto &command = *it;
    if (command.find(text) != std::string::npos) {
      auto *completion =
          new char[command.size()]; // NOLINT(cppcoreguidelines-owning-memory)
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
      static_cast<void>(
          std::snprintf(completion, command.size() + 1, "%s", command.c_str()));
      return completion;
    }
  }
  return nullptr;
}

char *Console::_command_generator_default(const char *text, int state) {
  auto commands = std::vector<std::string>();
  for (auto const &command : Console::get()._commands) {
    commands.emplace_back(command.first);
  }
  return _command_generator(text, state, commands);
}

char *Console::_command_generator_visualize(const char *text, int state) {
  return _command_generator(
      text, state, {"exec", "noexec", "pqp", "lqp", "unoptlqp", "joins"});
}

char *Console::_command_generator_setting(const char *text, int state) {
  return _command_generator(text, state, {"scheduler"});
}

char *Console::_command_generator_setting_scheduler(const char *text,
                                                    int state) {
  return _command_generator(text, state, {"on", "off"});
}

} // namespace storage

int main(int argc, char **argv) {

  using Return = storage::Console::ReturnCode;
  auto &console = storage::Console::get();

  // Bind CTRL-C to behaviour specified in Console::handle_signal.
  static_cast<void>(std::signal(SIGINT, &storage::Console::handle_signal));

  console.set_prompt("> ");
  console.set_logfile("console.log");
  console.set_console_path(argv[0]);

  // Load command history
  console.load_history(".repl_history");

  // Timestamp dump only to logfile
  console.out("--- Session start --- " + current_timestamp() + "\n", false);

  // TODO(anyone): Use std::to_underlying(ReturnCode::Ok) once we use C++23.
  auto return_code = Return::Ok;

  // Display usage if too many arguments are provided.
  if (argc > 1) {
    return_code = Return::Quit;
    console.out("Usage:\n");
    console.out("  ./hyriseConsole [SCRIPTFILE] - Start the interactive SQL "
                "interface.\n");
    console.out("                                 Execute script if specified "
                "by SCRIPTFILE.\n");
  }

  // Display welcome message if console started normally.
  if (argc == 1) {
    console.out("HYRISE SQL Interface\n");
    console.out("Type 'help' for more information.\n\n");

    console.out("RapidDB is running a ");
#ifdef _DEBUG
    console.out(ANSI_COLOR_RED "(debug)" ANSI_COLOR_RESET);
#else
    console.out(ANSI_COLOR_GREEN "(release)" ANSI_COLOR_RESET);
#endif
    console.out(" build.\n\n");
  }

  storage::SystemMgr::InitSystem();

  // Set jmp_env to current program state in preparation for siglongjmp(2). See
  // comment on jmp_env for details.
  while (sigsetjmp(jmp_env, 1) != 0) {
  }

  // Main REPL loop.
  while (return_code != Return::Quit) {
    return_code = static_cast<storage::Console::ReturnCode>(console.read());
    if (return_code == Return::Ok) {
      console.set_prompt("> ");
    } else if (return_code == Return::Multiline) {
      console.set_prompt("... ");
    } else if (return_code == Return::Quit) {
      break;
    } else {
      console.set_prompt("!> ");
    }
  }

  storage::SystemMgr::CloseSystem();
}
