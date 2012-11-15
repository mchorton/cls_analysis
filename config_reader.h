#include <iostream>
#include <string>
using namespace std;
class config_reader {

private:
  string file;
  string delimiter;
  string factory_declaration;
  string data_declaration;
  string data_hist_declaration;
  string signal_declaration;
  string signal_hist_declaration;
  string background_declaration;
  string background_hist_declaration;
  string double_declaration;
  RooWorkspace *ws;
  
public:
  //constuctors
  config_reader();
  config_reader(const string filename, RooWorkspace *workspace);
  config_reader(const config_reader& cr);

  // Destructor
  ~config_reader();

  // Accessor Methods
  string getfile() const;

  // Search through the file for the first instance of str.  Then, copy
  // everything from there to delimiter (inclusive), and return it.
  string fetch_decl_string(string str);

  string strip_delimiter(string str);

  void factory_string(string str);
  // Reads through the file, 
  string strip_factory_declaration(string str);

  // Puts variables into the workspace, based on the config file.
  void factory_all();

  // return the workspace
  RooWorkspace * getWorkspace();

  string get_data_hist_file_name();
  string get_data_hist_name();
  string get_signal_hist_file_name();
  string get_signal_hist_name();
  string get_data_hist_file_name();
  string get_data_hist_name();
  
  string strip_bounds (string str, string bound1, string bound2);
  
  double find_double(string str);
};

