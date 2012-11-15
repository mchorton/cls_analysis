/*
 * Max Horton
 * Cern 2012 Summer Student
 * Caltech, Class of 2014
 * Maxwell.Christian.Horton@gmail.com
 *
 * This is a simple class created to read config files to draw
 * out information.  Contains methods specifically needed by
 * the workspace_preparer function to assist in the creation
 * of variables and pdfs that need to be put in the 
 * workspace to prepare for the calculator call.
 */


#include <iostream>
#include <fstream>
#include "config_reader.h"
#include <string>

using namespace RooFit;
using namespace RooStats;

// Default constructor
config_reader::config_reader() {
// We don't want anyone to actually use this constructor, so it won't do
// anything.
}

/*
 * The useful constructor.  We don't allow most of the variables to change
 * because they dictate how the config files are formatted.  We want the
 * config files to have the same format all the time.
 */
config_reader::config_reader(const string filename, 
                             RooWorkspace *workspace){
  // This string marks the end of every line of the config file that
  // isn't a comment.
  delimiter = ";";
  // If this tag is at the beginning of a line, a corresponding call to
  // workspace->factory() will be made in factory_all().
  factory_declaration = "make:";
  // These values are present from an earlier version in which .root files
  // were specified in the config file (now they are passed to
  // cls_analysis).  It is left in in case of a change back to original
  // methods.
  data_declaration = "data:";
  data_hist_declaration = "data_hist:";
  signal_declaration = "signal:";
  signal_hist_declaration = "signal_hist:";
  background_declaration = "background:";
  background_hist_declaration = "background_hist:";
  // A tag used to identify values that will be considered as doubles.
  double_declaration = "double:";

  // Name of the config file associated with the config_reader
  file = filename;
  // Name of the workspace into which the config_reader will manufacture
  // values (e.g. in factory_all()
  ws = workspace;
}

// Copy constructor
config_reader::config_reader(const config_reader &cr){
  file = cr.getfile();
}

// Destructor
config_reader::~config_reader() {
}

// Accessor Methods
string config_reader::getfile() const{
  return file;
}

/*
 * Retrieve the character string starting from str, and ending with the
 * next occurence of the delimiter.
 * Note that whole file will be searched.
 */
string config_reader::fetch_decl_string(string str){
  string value;
  string line;
  ifstream config_file(file.c_str());
  if (!config_file.is_open()){
    cout << "ERROR: Given config file could not be opened.  File name: "
         << file << " does not exist." << endl;
    exit(1);
  }
  while (true){
    int loc;
    int end;
    getline(config_file, line);
    /* Nothing to do here, if line is nothing */
    if (config_file.eof())
      break;
    value = line;
   /* Now, we have the line.  We must check and see if it has the
    * expected char string.  Naively assume the string only appears once*/   
    loc = value.find(str);
    if (loc != value.npos){
      end = value.find(delimiter);
      if (end != value.npos){
        value = value.substr(loc, end-loc+delimiter.length());
        return value;
      }
    }
  }
  // If value is not found, throw an error (we don't want the program to
  // continue to execute if the config file is not properly formatted!)
  exit(1);
}

/*
 * Removes the delimiter from the given string str.  Returns the stripped
 * string.
 */
string config_reader::strip_delimiter(string str){
  return str.substr(0, str.length() - delimiter.length());
}

/*
 * Strips the factory declaration from the string.  Returns the stripped
 * string.
 */
string config_reader::strip_factory_declaration(string str){
  return str.substr(factory_declaration.length(), str.length()-1);
}

// Makes a factory call in the workspace on the string.
void config_reader::factory_string(string str){
  ws->factory(str.c_str());
}

/*
 * Find all lines with factory declaration in them.  Cut out the part 
 * of the string between "make:" and the delimiter, and call the 
 * workspace factory method on it.
 */
void config_reader::factory_all(){
  string value;
  string line;
  ifstream config_file(file.c_str());
  if (!config_file.is_open()){
    cout << "ERROR: Given config file could not be opened.  File name: "
         << file << " does not exist." << endl;
    exit(1);
  }
  while (true){
    int loc;
    int end;
    getline(config_file, line);
    /* Nothing to do here, if line is nothing */
    if (config_file.eof())
      break;
    value = line;
   /* Now, we have the line.  We must check and see if it has the
    * expected char string.  Naively assume the string only appears once*/   
    loc = value.find(factory_declaration);
    if (loc != value.npos){
      end = value.find(delimiter);
      if (end != value.npos){
        // Pull out the value, including declaration / delimiter
        value = value.substr(loc, end-loc+delimiter.length());
        // Remove declaration and delimiter
        value = strip_factory_declaration(value);
        value = strip_delimiter(value);
        // Put the variable into the workspace using the factory method.
        factory_string(value); 
      }
    }
  }
}

// Accessor
RooWorkspace* config_reader::getWorkspace(){
  return ws;
}

// Returns decl, ending at the occurence of bound.
string config_reader::findstrip(string decl, string bound){
  return strip_bounds(fetch_decl_string(decl), decl, bound);
}

string config_reader::strip_bounds (string str, string bound1, string bound2){
  val = str.substr(bound1.length(), str.length() - bound1.length() -
                   bound2.length());
  return val;
}

// Get values from a config file.
string config_reader::get_data_hist_file_name(){
  return findstrip(data_declaration, delimiter);
}
string config_reader::get_data_hist_name(){
  return findstrip(data_hist_declaration, delimiter);
}
string config_reader::get_signal_hist_file_name(){
  return findstrip(signal_declaration, delimiter);
}
string config_reader::get_signal_hist_name(){
  return findstrip(signal_hist_declaration, delimiter);
}
string config_reader::get_background_hist_file_name(){
  return findstrip(background_declaration, delimiter);
}
string config_reader::get_background_hist_name(){
  return findstrip(background_hist_declaration, delimiter);
}


/*
 * Looks through the config file for strings formatted with the double
 * declaration at the front, and with str afterwords, and ending in a
 * semicolon.  Such as: double:value78.98;  This method returns the double
 * (i.e. 78.98)
 */
double config_reader::find_double(string str){

  // Copy the double_declaration.  Otherwise, append will modify it!
  string copy = double_declaration;
  string copy2 = copy.append(str);
  string copy3 = fetch_decl_string(copy2);
  string copy4 = strip_bounds(copy3, copy2, delimiter);
  return atof(copy4.c_str());
}
