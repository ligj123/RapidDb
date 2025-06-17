#pragma once

#include "../src/statement/StmtResult.h"

#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace storage {
template <typename T> class Singleton {
public:
  inline static T &get() {
    static T instance;
    return instance;
  }

  virtual ~Singleton() {}

protected:
  // If you need to overwrite the constructor make sure to friend this Singleton
  // class. Otherwise it cannot call the protected constructor of a derived
  // class.
  Singleton() {}

  Singleton &operator=(Singleton &&) = default;
};

/*
 * SQL REPL Console for Hyrise, built on GNU readline.
 * https://cnswww.cns.cwru.edu/php/chet/readline/rltop.html
 */
class Console : public Singleton<Console> {
public:
  using CommandFunction = std::function<int(const std::string &)>;
  using RegisteredCommands = std::unordered_map<std::string, CommandFunction>;

  enum ReturnCode { Multiline = -2, Quit = -1, Ok = 0, Error = 1 };

  Console();
  ~Console();
  /*
   * Prompts user for one line of input, evaluates the given input, and prints
   * out the result.
   *
   * @returns ReturnCode::Quit if Console should be terminated,
   *          ReturnCode::Error if an error occured,
   *          ReturnCode::Multiline if the last command ended with a '\' and
   * should be continued next line, ReturnCode::Ok if the input was
   * evaluated/executed correctly.
   */
  int read();

  int execute_script(const std::string &filepath);

  /*
   * Register a custom command which can be called from the console.
   */
  void register_command(const std::string &name, const CommandFunction &func);
  RegisteredCommands &commands() { return _commands; }

  /*
   * Set prompt which is shown at the beginning of each line.
   */
  void set_prompt(const std::string &prompt);

  /*
   * Set logfile path.
   */
  void set_logfile(const std::string &logfile);

  /*
   * Set the executable path. Used to call external programs, such as data
   * generators.
   */
  void set_console_path(const std::string &path);

  /*
   * Load command history from history file.
   */
  void load_history(const std::string &history_file);

  /*
   * Prints to the log_file (and the console).
   *
   * @param output        The text that should be printed.
   * @param console_print If set to false, then \p output gets printed ONLY to
   * the log_file.
   */
  void out(const std::string &output, bool console_print = true);

  /*
   * Handler for SIGINT signal (caused by CTRL-C key sequence).
   * Resets the Console state and clears the current line.
   */
  static void handle_signal(int sig);

protected:
  /*
   * Non-public constructor, since Console is a Singleton.
   */

  /*
   * Evaluates given input string. Calls either _eval_command or _eval_sql.
   */
  int _eval(const std::string &input);

  /*
   * Evaluates given Console command.
   */
  static int _eval_command(const CommandFunction &func,
                           const std::string &command);

  /*
   * Evaluates given SQL statement using hyrise::SqlQueryTranslator.
   */
  int _eval_sql(const std::string &sql);

  // Command functions, registered to be called from the Console
  int _exit(const std::string & /*args*/);
  int _help(const std::string & /*args*/);

  int _print_current_working_directory();

  // GNU readline interface to our commands
  static char **_command_completion(const char *text, const int start,
                                    const int /*end*/);
  static char *_command_generator(const char *text, int state,
                                  const std::vector<std::string> &commands);
  static char *_command_generator_default(const char *text, int state);
  static char *_command_generator_visualize(const char *text, int state);
  static char *_command_generator_setting(const char *text, int state);
  static char *_command_generator_setting_scheduler(const char *text,
                                                    int state);

  std::string _prompt;
  std::string _multiline_input;
  std::string _history_file;
  RegisteredCommands _commands;
  std::ostream _out;
  std::ofstream _log;
  bool _verbose;
  bool _pagination_active;
  std::string _path;
  bool _binary_caching{true};

  StmtResult _stmtRes;
  uint32_t _stmtId{0};
  uint32_t _exprId{0};
};

} // namespace storage
