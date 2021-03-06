

//    --------------------------------------------------------------------
//
//    This file is part of Luna.
//
//    LUNA is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Luna is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Luna. If not, see <http://www.gnu.org/licenses/>.
//
//    Please see LICENSE.txt for more details.
//
//    --------------------------------------------------------------------



#include "eval.h"

#include "luna.h"


extern logger_t logger;

extern writer_t writer;


//
// param_t 
//

void param_t::add( const std::string & option , const std::string & value ) 
{
  
  if ( opt.find( option ) != opt.end() ) 
    Helper::halt( option + " parameter specified twice, only one value would be retained" );
  
    opt[ option ] = value; 
}  


void param_t::add_hidden( const std::string & option , const std::string & value ) 
{
  add( option , value );
  hidden.insert( option );
}

int param_t::size() const 
{ 
  // handle hidden things...
  return opt.size() - hidden.size();
}

void param_t::parse( const std::string & s )
{
  std::vector<std::string> tok = Helper::quoted_parse( s , "=" );
  if ( tok.size() == 2 )     add( tok[0] , tok[1] );
  else if ( tok.size() == 1 ) add( tok[0] , "T" );
  else // ignore subsequent '=' signs in 'value'  (i.e. key=value=2  is 'okay', means "value=2" is set to 'key')
    {
      std::string v = tok[1];
      for (int i=2;i<tok.size();i++) v += "=" + tok[i];
      add( tok[0] , v );
    }
}

void param_t::update( const std::string & id , const std::string & wc )
{
  // replace all instances of 'globals::indiv_wildcard' with 'id'
  // for all values
  std::map<std::string,std::string>::iterator ii = opt.begin();
  while ( ii != opt.end() ) 
    {
      std::string v = ii->second;
      bool changed = false;
      while ( v.find( wc ) != std::string::npos )
	{
	  int p = v.find( wc );
	  v = v.substr( 0 , p ) + id + v.substr(p+1);
	  changed = true;
	}
      if ( changed ) ii->second = v;
      ++ii;
    }
  
}

void param_t::clear() 
{ 
  opt.clear(); 
  hidden.clear(); 
} 

bool param_t::has(const std::string & s ) const 
{
  return opt.find(s) != opt.end(); 
} 

  
// bool param_t::has(const std::string & s , std::string * mstr ) const 
// {
//   std::string m = match( s );
//   if ( mstr != NULL ) *mstr = m;
//   return m != "";
//   //  return opt.find(s) != opt.end(); 
// } 

// bool param_t::matches( const std::string & inp , const std::string & tmp ) const
// {
//   // input, e.g.  signals
//   // ref    e.g.  sig       i.e. ref/template must be unique
//   return Helper::iequals( inp.substr( 0 , tmp.size() ) , tmp ) ;
// }

// std::string param_t::match( const std::string & str ) const 
// {
//   std::string ms = "";
//   int m = 0;
//   std::map<std::string,std::string>::const_iterator ii = opt.begin();
//   while ( ii != opt.end() )
//     {
//       // partial, case-insensitive match
//       if ( matches( ii->first , str ) )
// 	{
// 	  ++m;
// 	  if ( m > 1 ) Helper::halt( str + " matches multiple argumens" );
// 	  ms = ii->first;
// 	}
//       ++ii;
//     }
//   if ( m == 1 ) return ms;
//   return "";
// }

std::string param_t::value( const std::string & s ) const 
{ 
  if ( has( s ) )
    return opt.find( s )->second;
  else
    return "";
}

bool param_t::single() const 
{ 
  return size() == 1; 
}

std::string param_t::single_value() const 
{ 
  if ( ! single() ) Helper::halt( "no single value" ); 
  
  std::map<std::string,std::string>::const_iterator ii = opt.begin();
      
  while ( ii != opt.end() ) 
    {
      if ( hidden.find( ii->first ) == hidden.end() ) return ii->first;
      ++ii;
    }
  return ""; // should not happen
}

std::string param_t::requires( const std::string & s ) const
{
  if ( ! has(s) ) Helper::halt( "command requires parameter " + s );
  return value(s);
}

int param_t::requires_int( const std::string & s ) const
{
  if ( ! has(s) ) Helper::halt( "command requires parameter " + s );
  int r;
  if ( ! Helper::str2int( value(s) , &r ) ) 
    Helper::halt( "command requires parameter " + s + " to have an integer value" );
  return r;
}

double param_t::requires_dbl( const std::string & s ) const
{
  if ( ! has(s) ) Helper::halt( "command requires parameter " + s );
  double r;
  if ( ! Helper::str2dbl( value(s) , &r ) ) 
    Helper::halt( "command requires parameter " + s + " to have a numeric value" );
  return r;
}

std::string param_t::dump( const std::string & indent , const std::string & delim ) const
{
  std::map<std::string,std::string>::const_iterator ii = opt.begin();
  int sz = opt.size();
  int cnt = 1;
  std::stringstream ss;
  while ( ii != opt.end() ) 
    {
      if ( cnt == sz )
	ss << indent << ii->first << "=" << ii->second; 
      else
	ss << indent << ii->first << "=" << ii->second << delim; 
      ++cnt;
      ++ii;
    }
  return ss.str();
}

std::set<std::string> param_t::strset( const std::string & k , const std::string delim ) const
{
  std::set<std::string> s;
  if ( ! has(k) ) return s;
  std::vector<std::string> tok = Helper::quoted_parse( value(k) , delim );
  for (int i=0;i<tok.size();i++) s.insert( Helper::unquote( tok[i]) );
  return s;
}

std::vector<std::string> param_t::strvector( const std::string & k , const std::string delim ) const
{
  std::vector<std::string> s;
  if ( ! has(k) ) return s;
  std::vector<std::string> tok = Helper::quoted_parse( value(k) , delim );
  for (int i=0;i<tok.size();i++) s.push_back( Helper::unquote( tok[i]) );
  return s;
}

std::vector<double> param_t::dblvector( const std::string & k , const std::string delim ) const
{
  std::vector<double> s;
  if ( ! has(k) ) return s;
  std::vector<std::string> tok = Helper::quoted_parse( value(k) , delim );
  for (int i=0;i<tok.size();i++) 
    {
      std::string str = Helper::unquote( tok[i]);
      double d = 0;
      if ( ! Helper::str2dbl( str , &d ) ) Helper::halt( "Option " + k + " requires a double value(s)" );
      s.push_back(d); 
    }
  return s;
}

std::vector<int> param_t::intvector( const std::string & k , const std::string delim ) const
{
  std::vector<int> s;
  if ( ! has(k) ) return s;
  std::vector<std::string> tok = Helper::quoted_parse( value(k) , delim );
  for (int i=0;i<tok.size();i++) 
    {
      std::string str = Helper::unquote( tok[i]);
      int d = 0;
      if ( ! Helper::str2int( str , &d ) ) Helper::halt( "Option " + k + " requires an integer value(s)" );
      s.push_back(d);
    }
  return s;
}


std::set<std::string> param_t::keys() const
{
  std::set<std::string> s;
  std::map<std::string,std::string>::const_iterator ii = opt.begin();
  while ( ii != opt.end() )
    {
      s.insert( ii->first );
      ++ii;
    }
  return s;
}



//
// cmd_t
//


cmd_t::cmd_t() 
{
  reset();
  error = ! read();
}

cmd_t::cmd_t( const std::string & str ) 
{
  reset();
  error = ! read( &str , true ); 
}

void cmd_t::add_cmdline_cmd( const std::string & c ) 
{
  cmdline_cmds.append( c + " " );
}

void cmd_t::reset() 
{
  cmds.clear();
  params.clear();
  line = "";
  error = false;
  will_quit = false;
}


void cmd_t::clear_static_members() 
{
  input = "";
  cmdline_cmds = "";
  stout_file = "";
  append_stout_file = false;
  
  vars.clear();
  signallist.clear();
  label_aliases.clear();
  primary_alias.clear();
}

bool cmd_t::empty() const 
{ 
  return will_quit; 
}

bool cmd_t::valid() const 
{    
  if ( error ) return false;
  /* for (int c=0;c<cmds.size();c++) */
  /*   if ( commands.find( cmds[c] ) == commands.end() ) return false; */
  return true;
}

bool cmd_t::badline() const 
{ 
  return error; 
} 

std::string cmd_t::offending() const 
{ 
  return ( error ? line : "" ); 
}

int cmd_t::num_cmds() const 
{ 
  return cmds.size(); 
}

std::string cmd_t::cmd(const int i) 
{ 
    return cmds[i]; 
}

param_t & cmd_t::param(const int i) 
{ 
  return params[i]; 
}

bool cmd_t::process_edfs() const
{
  // all commands process EDFs, /except/ the following
  if ( cmds.size() == 1 
       && ( cmds[0] == "" 
	    || cmds[0] == "." 
	    || Helper::iequals( cmds[0] , "DUMMY" ) 
	    || Helper::iequals( cmds[0] , "INTERVALS" )
	    ) )
    return false;
  return true;    
}

bool cmd_t::is( const int n , const std::string & s ) const
{
  if ( n < 0 || n >= cmds.size() ) Helper::halt( "bad command number" );
  return Helper::iequals( cmds[n] , s );
}
  
std::string cmd_t::data() const 
{ 
  return input; 
} 

bool cmd_t::quit() const 
{ 
  return will_quit; 
}

  
void cmd_t::quit(bool b) 
{ 
  will_quit = b; 
}


void cmd_t::signal_alias( const std::string & s )
{

  // the primary alias can occur multiple times, and have multiple 
  // labels that are mapped to it
  
  // however: two rules
  // 1. many-to-one mapping means the same label cannot have multiple primary aliases
  // 2. following, and on principle of no transitive properties, alias cannot have alias

  // X|Y|Z
  // X|A|B
  
  // W|A  bad, A already mapped
  // V|X  bad, X already mapped
  // i.e. things can only occur once in the RHS, or multiple times in the LHS
  
  
  // format canonical|alias1|alias2 , etc.
  std::vector<std::string> tok = Helper::quoted_parse( s , "|" );    
  if ( tok.size() < 2 ) Helper::halt( "bad format for signal alias:  canonical|alias 1|alias 2" );
  const std::string primary = Helper::unquote( tok[0] );
  for (int j=1;j<tok.size();j++) 
    {
      
      // impose rules
      const std::string mapped = Helper::unquote( tok[j] ) ;
      
      if ( primary_alias.find( mapped ) != primary_alias.end() )
	Helper::halt( mapped + " specified as both primary alias and mapped term" );

      if ( label_aliases.find( mapped ) != label_aliases.end() )
	if ( primary != label_aliases[ mapped ] )  
	  Helper::halt( mapped + " specified twice in alias file w/ different primary aliases" );

      // otherwise, set 
      label_aliases[ mapped ] = primary;

      primary_alias[ primary ].push_back( mapped );
    }
  
}


const std::set<std::string> & cmd_t::signals() 
{ 
  return signallist; 
}
  
void cmd_t::clear_signals() 
{ 
  signallist.clear(); 
}

std::string cmd_t::signal_string() 
{
  
  if ( signallist.size() == 0 ) return "*"; // i.e. all signals
  
  std::stringstream ss;
  std::set<std::string>::iterator ii = signallist.begin();
  while ( ii != signallist.end() )
    {
      if ( ii != signallist.begin() ) ss << ",";
      ss << *ii;	  
      ++ii;
    }
  return ss.str();
}
  

void cmd_t::populate_commands() 
{
  commands.insert( "VALIDATE" );
  commands.insert( "HEADERS" );
  commands.insert( "SUMMARY" );
  commands.insert( "PSD" );
  commands.insert( "SPINDLES" );
  commands.insert( "RMS" );
  commands.insert( "MASK" );
  commands.insert( "CWT" );
  commands.insert( "RESCALE" );
  commands.insert( "DUMP" );
  commands.insert( "DUMP-MASK" );
  commands.insert( "DUMMY" );
  commands.insert( "ARTIFACTS" );
  commands.insert( "TIME-TRACK" );
}


// ----------------------------------------------------------------------------------------
//
// Process commands from STDIN
//
// ----------------------------------------------------------------------------------------

void cmd_t::replace_wildcards( const std::string & id )
{
  // replace in all 'params' any instances of 'globals::indiv_wildcard' with 'id'
  params = original_params;
  for (int p = 0 ; p < params.size(); p++ ) params[p].update( id , globals::indiv_wildcard );
}



bool cmd_t::read( const std::string * str , bool silent )
{
  
  bool cmdline_mode = str == NULL;   
  
  if ( std::cin.eof() && cmdline_mode ) return false;
  
  if ( (!cmdline_mode) && str->size() == 0 ) return false;
  
  reset();
  
  // CMD param=1 p=1,2,3 f=true out=o1 & CMD2 etc ; 
  
  // EITHER read from std::cin, 
  // OR          from -s command line
  // OR          from a string (e.g. lunaR)
  
  // Commands are delimited by & symbols (i.e. multi-line statements allowed)
  
  std::istringstream allinput;
  
  if ( ! cmdline_mode ) // read from 'str', such as R interface
    {
      
      // split by commands ('&' symbols), but allowing for & witin eval expressions 
      std::vector<std::string> tok = Helper::quoted_parse( *str , "&" );
      
      std::stringstream ss;
      
      for (int l=0;l<tok.size();l++)
	{
	  if ( tok[l] == "" ) continue;
	  if ( l != 0 ) ss << " & ";
	  ss << tok[l];
	}     
      allinput.str( ss.str() );

    }

  // read from std::cin
  else if ( cmd_t::cmdline_cmds == "" )
    {
      std::stringstream ss;
      bool first_cmd = true;
      while ( 1 )
	{
	  std::string s;
	  std::getline( std::cin , s , '\n' );
	  if ( std::cin.eof() ) break;
	  if ( s == "" ) continue;	  
	  
	  // is this a continuation line?
	  bool continuation = s[0] == ' ' || s[0] == '\t';

	  // only read up to a % comment, although this may be quoted
	  if ( s.find( "%" ) != std::string::npos ) 
	    {
	      bool inquote = false;
	      int comment_start = -1;
	      for (int i=0;i<s.size();i++)
		{
		  if ( s[i] == '"' || s[i] == '#' ) inquote = ! inquote;
		  if ( s[i] == '%' && ! inquote ) { comment_start = i; break; }
		}
	      
	      // remove comment
	      if ( comment_start != -1 )
		s = s.substr( 0 , comment_start );
	    }

	  // trim leading/trailing whitespace
	  s = Helper::ltrim( s );
	  s = Helper::rtrim( s );

	  // anything left to add?
	  if ( s.size() > 0 ) 
	    {
	      if ( ! continuation ) 
		{
		  if ( ! first_cmd ) ss << " & ";		  
		  first_cmd = false;
		}
	      else 
		{
		  ss << " "; // spacer
		}	      
	      // add actual non-empty command
	      ss << s ;	      
	    }

	}

      allinput.str( ss.str() );
    }

  // read from -s string
  else 
    allinput.str( cmd_t::cmdline_cmds );
      
  // take everything
  line = allinput.str();
  
  // change any '&' (back) to '\n', unless they are quoted (" or #)
  bool inquote = false;
  for (int i=0;i<line.size();i++) 
    {
      if ( line[i] == '#' || line[i] == '"' ) inquote = ! inquote;
      else if ( line[i] == '&' ) 
	{
	  if ( ! inquote ) line[i] = '\n';
	}
    }

  // skip comments between commands if line starts with '%' (or '\n')
  bool recheck = true;
  while (recheck)
    {     
      if ( line[0] == '%' || line[0] == '\n' ) 
	{
	  line = line.substr( line.find("\n")+1);
	}
      else recheck = false;
    }
  
  // swap in any variables
  Helper::swap_in_variables( &line , vars );

  
  std::vector<std::string> tok = Helper::quoted_parse( line , "\n" );
  if ( tok.size() == 0 ) 
    {
      quit(true);
      return false;
    }

  // command(s)
  for (int c=0;c<tok.size();c++)
    {      
      std::vector<std::string> ctok = Helper::quoted_parse( tok[c] , "\t " );
      if ( ctok.size() < 1 ) return false;
      cmds.push_back( ctok[0] );
      param_t param;
      for (int j=1;j<ctok.size();j++) param.parse( ctok[j] );
      params.push_back( param );
    }
  
  // make a copy of the 'orginal' params
  original_params = params;

  // summary

  logger << "input(s): " << input << "\n";
  logger << "output  : " << writer.name() << "\n";
  
  if ( signallist.size() > 0 )
    {
      logger << "signals :";
      for (std::set<std::string>::iterator s=signallist.begin();
	   s!=signallist.end();s++) 
	logger << " " << *s ;
      logger << "\n";
    }
  
  for (int i=0;i<cmds.size();i++)
    {
      if ( i==0 ) 
	logger << "commands: ";
      else
	logger << "        : ";
      
      logger << "c" << i+1 
	     << "\t" << cmds[i] << "\t"
	     << params[i].dump("","|") 
	     << "\n";
    }
  

  return true;
}



//
// Evaluate commands
//

bool cmd_t::eval( edf_t & edf ) 
{

  //
  // Loop over each command
  //
  
  for ( int c = 0 ; c < num_cmds() ; c++ )
    {	        
      
      // was a problem flag raised when loading the EDF?
      
      if ( globals::problem ) return false;
      
      //
      // If this particular command did not explicitly specify
      // signals, then add all
      //
      
      if ( ! param(c).has( "sig" ) )
	param(c).add_hidden( "sig" , signal_string() );
      

      //
      // Print command
      //

      logger << " ..................................................................\n"
	     << " CMD #" << c+1 << ": " << cmd(c) << "\n";
      
      
      writer.cmd( cmd(c) , c+1 , param(c).dump( "" , " " ) );

      // use strata to keep track of tables by commands

      writer.level( cmd(c) , "_" + cmd(c) );
      
      
      //
      // Now process the command
      //
      
      if      ( is( c, "WRITE" ) )        proc_write( edf, param(c) );
      else if ( is( c, "SUMMARY" ) )      proc_summaries( edf , param(c) );
      else if ( is( c, "HEADERS" ) )      proc_headers( edf , param(c) );
      

      else if ( is( c, "DESC" ) )         proc_desc( edf , param(c) );
      else if ( is( c, "STATS" ) )        proc_stats( edf , param(c) );
      
      else if ( is( c, "REFERENCE" ) )    proc_reference( edf , param(c) );
      else if ( is( c, "FLIP" ) )         proc_flip( edf , param(c) );
      else if ( is( c, "uV" ) )           proc_scale( edf , param(c) , "uV" ); 
      else if ( is( c, "mV" ) )           proc_scale( edf , param(c) , "mV" );
      else if ( is( c, "RECORD-SIZE" ) )  proc_rerecord( edf , param(c) );
      
      else if ( is( c, "TIME-TRACK" ) )   proc_timetrack( edf, param(c) );
      else if ( is( c, "CONTIN" ) )       proc_continuous( edf, param(c) );

      else if ( is( c, "STAGE" ) )        proc_sleep_stage( edf , param(c) , false );
      else if ( is( c, "HYPNO" ) )        proc_sleep_stage( edf , param(c) , true );
      
      else if ( is( c, "TSLIB" ) )        pdc_t::construct_tslib( edf , param(c) );
      else if ( is( c, "SSS" ) )          pdc_t::simple_sleep_scorer( edf , param(c) );
      else if ( is( c, "EXE" ) )          pdc_t::similarity_matrix( edf , param(c) );
      
      else if ( is( c, "DUMP" ) )         proc_dump( edf, param(c) );
      else if ( is( c, "DUMP-RECORDS" ) ) proc_record_dump( edf , param(c) );
      else if ( is( c, "DUMP-EPOCHS" ) )  proc_epoch_dump( edf, param(c) ); // REDUNDANT; use ANNOTS epoch instead

      else if ( is( c, "ANNOTS" ) )       proc_list_all_annots( edf, param(c) );
      else if ( is( c, "COUNT-ANNOTS" ) ) proc_list_annots( edf , param(c) );

      else if ( is( c, "MATRIX" ) )       proc_epoch_matrix( edf , param(c) );
      else if ( is( c, "RESTRUCTURE" ) || is( c, "RE" ) )  proc_restructure( edf , param(c) );
      else if ( is( c, "SIGNALS" ) )      proc_drop_signals( edf , param(c) );
      else if ( is( c, "COPY" ) )         proc_copy_signal( edf , param(c) );
      else if ( is( c, "RMS" ) || is( c, "SIGSTATS" ) ) proc_rms( edf, param(c) );
      else if ( is( c, "MSE" ) )          proc_mse( edf, param(c) );
      else if ( is( c, "LZW" ) )          proc_lzw( edf, param(c) );
      else if ( is( c, "ZR" ) )           proc_zratio( edf , param(c) );
      else if ( is( c, "ANON" ) )         proc_anon( edf , param(c) );
      else if ( is( c ,"EPOCH" ) )        proc_epoch( edf, param(c) );
      else if ( is( c ,"SLICE" ) )        proc_slice( edf , param(c) , 1 );
      
      else if ( is( c, "EVAL" ) )         proc_eval( edf, param(c) );
      else if ( is( c, "MASK" ) )         proc_mask( edf, param(c) );

      else if ( is( c, "FILE-MASK" ) )    proc_file_mask( edf , param(c) ); // not supported/implemented
      else if ( is( c, "DUMP-MASK" ) )    proc_dump_mask( edf, param(c) );
      
      else if ( is( c, "EPOCH-ANNOT" ) )  proc_file_annot( edf , param(c) );
      else if ( is( c, "EPOCH-MASK" ) )   proc_epoch_mask( edf, param(c) );
      
      
      else if ( is( c, "FILTER" ) )       proc_filter( edf, param(c) );      
      else if ( is( c, "FILTER-DESIGN" )) proc_filter_design( edf, param(c) );
      else if ( is( c, "CWT-DESIGN" ) )   proc_cwt_design( edf , param(c) );
      else if ( is( c, "CWT" ) )          proc_cwt( edf , param(c) );
      else if ( is( c, "HILBERT" ) )      proc_hilbert( edf , param(c) );

      //	  else if ( is( c, "LEGACY-FILTER" )) proc_filter_legacy( edf, param(c) );
      else if ( is( c, "TV" ) )           proc_tv_denoise( edf , param(c) );
      
      else if ( is( c, "COVAR" ) )        proc_covar( edf, param(c) );
      else if ( is( c, "PSD" ) )          proc_psd( edf, param(c) );	  
      else if ( is( c, "MTM" ) )          proc_mtm( edf, param(c) );
      else if ( is( c, "1FNORM" ) )       proc_1overf_norm( edf, param(c) );
      
      else if ( is( c, "FIP" ) )          proc_fiplot( edf , param(c) );
      
      else if ( is( c, "COH" ) )          proc_coh( edf , param(c) );
      else if ( is( c, "LECGACY-COH" ) )  proc_coh_legacy( edf , param(c) );
      else if ( is( c, "CORREL" ) )       proc_correl( edf , param(c) );
      else if ( is( c, "ED" ) )           proc_elec_distance( edf , param(c) );
      else if ( is( c, "ICA" ) )          proc_ica( edf, param(c) );
      else if ( is( c, "L1OUT" ) )        proc_leave_one_out( edf , param(c) );
      
      else if ( is( c, "EMD" ) )          proc_emd( edf , param(c) );
  
      else if ( is( c, "MI" ) )           proc_mi( edf, param(c) );
      else if ( is( c, "HR" ) )           proc_bpm( edf , param(c) );
      else if ( is( c, "SUPPRESS-ECG" ) ) proc_ecgsuppression( edf , param(c) );
      else if ( is( c, "PAC" ) )          proc_pac( edf , param(c) );
      else if ( is( c, "CFC" ) )          proc_cfc( edf , param(c) );
      else if ( is( c, "TAG" ) )          proc_tag( param(c) );
      else if ( is( c, "RESAMPLE" ) )     proc_resample( edf, param(c) );
      else if ( is( c, "SPINDLES" ) )     proc_spindles( edf, param(c) );	  
      else if ( is( c, "POL" ) )          proc_polarity( edf, param(c) );	  
      
      else if ( is( c, "SW" ) || is( c, "SLOW-WAVES" ) ) proc_slowwaves( edf, param(c) );
      else if ( is( c, "ARTIFACTS" ) )    proc_artifacts( edf, param(c) );
      else if ( is( c, "SPIKE" ) )        proc_spike( edf , param(c) );
      
      else 
	{
	  Helper::halt( "did not recognize command: " + cmd(c) );
	  return false; 
	}
      
      
      //
      // Was a problem flag set?
      //
      
      if ( globals::problem ) 
	{
	  logger << "**warning: the PROBLEM flag was set, skipping to next EDF...\n";

	  return false;
	}
         
     
      writer.unlevel( "_" + cmd(c) );

    } // next command
  


  return true;
}



//
// ----------------------------------------------------------------------------------------
//
// Wrapper (proc_*) functions below that drive specific commands
//
// ----------------------------------------------------------------------------------------
//


// HEADERS : summarize EDF files

void proc_headers( edf_t & edf , param_t & param )
{
  edf.terse_summary();
}


// SUMMARY : summarize EDF files (verbose, human-readable)  

void proc_summaries( edf_t & edf , param_t & param )
{
  std::cout << "EDF filename   : " << edf.filename << "\n" 
	    << edf.header.summary() << "\n"
	    << "----------------------------------------------------------------\n\n";
}


// DESC : very brief summary of contents 

void proc_desc( edf_t & edf , param_t & param )
{
  edf.description();
}

// STATS : get basic stats for an EDF

void proc_stats( edf_t & edf , param_t & param )
{
  edf.basic_stats( param );
}

// RMS/SIGSTATS : calculate root-mean square for signals 

void proc_rms( edf_t & edf , param_t & param )
{    
  rms_per_epoch( edf , param );
}

// MSE : calculate multi-scale entropy per epoch

void proc_mse( edf_t & edf , param_t & param )
{    
  mse_per_epoch( edf , param );
}

// LZW : compression index per epoch/signal
void proc_lzw( edf_t & edf , param_t & param )
{    
  lzw_per_epoch( edf , param );
}


// ZR : Z-ratio 
void proc_zratio( edf_t & edf , param_t & param )
{    
  std::string signal = param.requires( "sig" );

  staging_t staging;
  staging.zratio.calc( edf , signal );
}


// ARTIFACTS : artifact rejection using Buckelmueller et al. 

void proc_artifacts( edf_t & edf , param_t & param )	  
{
  std::string signal = param.requires( "sig" );
  annot_t * a = buckelmuller_artifact_detection( edf , param , signal );  
}


// LEGACY-FILTER : band-pass filter, band-pass filter only

// void proc_filter_legacy( edf_t & edf , param_t & param )	  
// {
//   //band_pass_filter( edf , param );
// }

// FILTER : general FIR

void proc_filter( edf_t & edf , param_t & param )	  
{
  dsptools::apply_fir( edf , param );
}


// -fir  from the command line
void proc_filter_design_cmdline()
{
  
  // expect parameters on STDIN
  
  param_t param;
  while ( ! std::cin.eof() )
    {
      std::string x;
      std::cin >> x;      
      if ( std::cin.eof() ) break;
      if ( x == "" ) continue;
      param.parse( x ); 
    }

  dsptools::design_fir( param );
  
}

// TV   total variation 1D denoising

void proc_tv_denoise( edf_t & edf , param_t & param )
{
  dsptools::tv( edf , param );
}

// CWT 
void proc_cwt( edf_t & edf , param_t & param )
{
  dsptools::cwt( edf , param );
}

// HILBERT 
void proc_hilbert( edf_t & edf , param_t & param )
{
  dsptools::hilbert( edf , param );
}

// -cwt  from the command line
void proc_cwt_design_cmdline()
{
  
  // expect parameters on STDIN
  
  param_t param;
  while ( ! std::cin.eof() )
    {
      std::string x;
      std::cin >> x;      
      if ( std::cin.eof() ) break;
      if ( x == "" ) continue;
      param.parse( x ); 
    }
  
  dsptools::design_cwt( param );
  
}


// FILTER-DESIGN : general FIR design

void proc_filter_design( edf_t & edf , param_t & param )	  
{
  dsptools::design_fir( param );
}

// CWT-DESIGN : CWT design

void proc_cwt_design( edf_t & edf , param_t & param )	  
{
  dsptools::design_cwt( param );
}


// RESAMPLE : band-pass filter

void proc_resample( edf_t & edf , param_t & param ) 
{
  dsptools::resample_channel( edf, param );
}

// PSD : calculate PSD 

void proc_psd( edf_t & edf , param_t & param )	  
{  
  std::string signal = param.requires( "sig" );
  annot_t * power = spectral_power( edf , signal , param );  
}

// MTM : calculate MTM 

void proc_mtm( edf_t & edf , param_t & param )	  
{  
  mtm::wrapper( edf , param );
}

// 1FNORM : normalization of signals for the 1/f trend

void proc_1overf_norm( edf_t & edf , param_t & param )	  
{  
  dsptools::norm_1overf( edf,  param ) ;
}

// FI-plot : frequency/interval plot

void proc_fiplot( edf_t & edf , param_t & param )	  
{  
  fiplot_wrapper( edf , param );
}

// TAG : analysis tag

void proc_tag( param_t & param )
{
  // either TAG tag=lvl/fac
  // or just TAG lvl/fac 

  if ( ! param.single() ) Helper::halt( "TAG requires a single argument" );

  if ( param.has( "tag" ) )
    set_tag( param.value( "tag" ) );
  else 
    set_tag( param.single_value() );
  
}

void set_tag( const std::string & t ) 
{
  globals::current_tag = t ; 

  if ( t != "." ) 
    logger << " setting analysis tag to [" << globals::current_tag << "]\n";

  if ( t == "." ) writer.tag( "." , "." );
  else
    {
      std::vector<std::string> tok = Helper::parse( globals::current_tag , "/" );
      if ( tok.size() != 2 ) Helper::halt( "TAG format should be factor/level" );
      writer.tag( tok[1] , tok[0] );
    }
}

// ANON : anonymize EDF 

void proc_anon( edf_t & edf , param_t & param )
{
  logger << " setting subject ID and start date to null ('.') for EDF " 
	 << edf.filename << "\n";
  
  edf.header.patient_id = ".";
  //edf.header.starttime = ".";
  edf.header.startdate = ".";
}


// DUMP : dump all data

void proc_dump( edf_t & edf , param_t & param )	  
{
  std::string signal = param.requires( "sig" );  
  edf.data_dumper( signal , param );	  
}
      

// EPOCH DUMP 

void proc_epoch_dump( edf_t & edf , param_t & param )
{
  // REDUNDANT ; command not documented
  std::set<std::string> * annots = NULL;
  if ( param.has( "annot" ) )
    {
      annots = new std::set<std::string>;
      *annots = param.strset( "annot" ); // parse comma-delim list
    }

  edf.data_epoch_dumper( param , annots );
}


// MATRIX 

void proc_epoch_matrix( edf_t & edf , param_t & param )
{  
  edf.epoch_matrix_dumper( param );
}


// INTERVALS : raw signal data from an interval list

void proc_intervals( param_t & param , const std::string & data )	  
{  

  std::string ints = param.requires( "intervals" );
  // e.g.: INTERVAL edfs=../scratch/edf.list 
  // intervals from STDIN
  dump_intervals( ints , data );
}



// COVAR : covariance between two signals (not implemented)

void proc_covar( edf_t & edf , param_t & param )
{  
  std::string signals1 = param.requires( "sig1" );
  std::string signals2 = param.requires( "sig2" );
  edf.covar(signals1,signals2);
}


// SPINDLES : spindle detection using CWT or bandpass/RMS

void proc_spindles( edf_t & edf , param_t & param )
{	

  // default to wavelet analysis
  std::string method = param.has( "method" ) ? param.value( "method" ) : "wavelet" ; 
  
  annot_t * a = NULL;

  if      ( method == "bandpass" ) a = spindle_bandpass( edf , param );
  else if ( method == "wavelet" ) a = spindle_wavelet( edf , param );
  else Helper::halt( "SPINDLE method not recognized; should be 'bandpass' or 'wavelet'" );

}

// POL : polarity check for EEG N2/N3 

void proc_polarity( edf_t & edf , param_t & param )
{	
  dsptools::polarity( edf , param );
}

// SW || SLOW-WAVES : detect slow waves, do time-locked FFT on rest of signal

void proc_slowwaves( edf_t & edf , param_t & param )
{	

  // find slow-waves
  slow_waves_t sw( edf , param );
  
}


// WRITE : write a new EDF to disk (but not annotation files, they 
// must always be anchored to the 'original' EDF

void proc_write( edf_t & edf , param_t & param )
{
    
  // add 'tag' to new EDF
  std::string filename = edf.filename;
  if ( Helper::file_extension( filename, "edf" ) || 
       Helper::file_extension( filename, "EDF" ) ) 
    filename = filename.substr(0 , filename.size() - 4 );
  
  filename += "-" + param.requires( "edf-tag" ) + ".edf";

  // optionally, allow directory change
  if ( param.has( "edf-dir" ) )
    {
      const std::string outdir = param.value("edf-dir");
      
      if ( outdir[ outdir.size() - 1 ] != globals::folder_delimiter ) 
	Helper::halt("edf-dir value must end in '" + std::string(1,globals::folder_delimiter) + "' to specify a folder" );

      int p=filename.size()-1;
      int v = 0;
      for (int j=p;j>=0;j--)
	{
	  if ( filename[j] == globals::folder_delimiter ) { v=j+1; break; }
	}
      filename = outdir + filename.substr( v );            
      
      // create folder if it does not exist
      std::string syscmd = "mkdir -p " + param.value( "edf-dir" );
      int retval = system( syscmd.c_str() );
      
    }

  if ( param.has("sample-list") )
    {	  
      std::string file = param.value("sample-list");
      
      // open/append
      logger << " appending " << filename << " to sample-list " << file << "\n";
      
      std::ofstream FL( file.c_str() , std::ios_base::app );
      FL << edf.id << "\t"
	 << filename << "\n";
      
      FL.close();
    }

  //
  // prep EDF for writing then write to disk
  //

  // arbitrary, but need epochs if not set it seems
  if ( !edf.timeline.epoched() ) 
    edf.timeline.set_epoch( 30 , 30 );

  // if a mask has been set, this will restructure the mask
  edf.restructure(); 

  bool saved = edf.write( filename );

  if ( saved ) 
    logger << " saved new EDF, " << filename << "\n";

}


// EPOCH : set epoch 

void proc_epoch( edf_t & edf , param_t & param )
{
  
  double dur = 0 , inc = 0;
  
  // default = 30 seconds, non-overlapping
  if ( ! ( param.has( "len" ) || param.has("dur") || param.has( "epoch" ) ) )
    {
      dur = 30; inc = 30;
    }
  else
    {
      
      if ( param.has( "epoch" ) )
	{
	  std::string p = param.requires( "epoch" );
	  std::vector<std::string> tok = Helper::parse( p , "," );
	  if ( tok.size() > 2 || tok.size() < 1 ) Helper::halt( "expcting epoch=length{,increment}" );
	  
	  if ( ! Helper::str2dbl( tok[0] , &dur ) ) Helper::halt( "invalid epoch length" );
	  if ( tok.size() == 2 ) 
	    {
	      if ( ! Helper::str2dbl( tok[1] , &inc ) ) 
		Helper::halt( "invalid epoch increment" );
	    }
	  else inc = dur;
	}
      
      else if ( param.has( "len" ) ) 
	{
	  dur = param.requires_dbl( "len" );	  
	  if ( param.has( "inc" ) ) 
	    inc = param.requires_dbl( "inc" );
	  else 
	    inc = dur;
	}

      else if ( param.has( "dur" ) ) 
	{
	  dur = param.requires_dbl( "dur" );	  
	  if ( param.has( "inc" ) ) 
	    inc = param.requires_dbl( "inc" );
	  else 
	    inc = dur;
	}

    }

  if ( param.has( "inc" ) )
    {
      inc = param.requires_dbl( "inc" );
    }


  // if already EPOCH'ed for a different record size, or increment,
  // then we should first remove all epochs; this will remove the
  // EPOCH-to-record mapping too
  
  if ( edf.timeline.epoched() 
       && ( ( ! Helper::similar( edf.timeline.epoch_length() , dur ) )
	    || ( ! Helper::similar( edf.timeline.epoch_inc() , inc ) ) ) )
    {
      logger << " epoch definitions have changed: original epoch mappings will be lost\n";
      edf.timeline.unepoch();
    }

  //
  // basic log info
  //

  
  int ne = edf.timeline.set_epoch( dur , inc );  
  
  logger << " set epochs, length " << dur << " (step " << inc << "), " << ne << " epochs\n";

  writer.value( "NE" , ne );
  writer.value( "DUR" , dur );
  writer.value( "INC" , inc );
  

  //
  // write more verbose information to db
  //
  
  if ( param.has( "verbose" ) )
    {

      // track clock time

      clocktime_t starttime( edf.header.starttime );
      
      bool hms = starttime.valid;
      
      
      edf.timeline.first_epoch();
      
      while ( 1 ) 
	{
	  
	  int epoch = edf.timeline.next_epoch();      
	  
	  if ( epoch == -1 ) break;
	  
	  interval_t interval = edf.timeline.epoch( epoch );
	  
	  //      std::cout << "epoch " << epoch << "\t" << interval.as_string() << "\n";
	  
	  // original encoding (i.e. to allows epochs to be connected after the fact
	  writer.epoch( edf.timeline.display_epoch( epoch ) );
	  
	  // if present, original encoding
	  writer.value( "E1" , epoch+1 );
	  
	  writer.value( "INTERVAL" , interval.as_string() );      
	  writer.value( "START"    , interval.start_sec() );
	  writer.value( "MID"      , interval.mid_sec() );
	  writer.value( "STOP"     , interval.stop_sec() );
	  
	  // original time-points
	  
	  if ( hms )
	    {
	      const double sec0 = interval.start * globals::tp_duration;
	      clocktime_t present = starttime;
	      present.advance( sec0 / 3600.0 );
	      std::string clocktime = present.as_string();
	      writer.value( "HMS" , clocktime );
	    }
	
	  
	}		  
      
  
      writer.unepoch();
      
    }


  //
  // any constraints on the min. number of epochs required? 
  //

  if ( param.has("require") )
    {
      int r = param.requires_int( "require" );
      if ( ne < r ) 
	{	  
	  logger << " ** warning for "  
		 << edf.id << " when setting EPOCH: "
		 << "required=" << r << "\t"
		 << "but observed=" << ne << "\n";
	  globals::problem = true;
	}
    }
}


// FILE-MASK : apply a mask from a file

void proc_file_mask( edf_t & edf , param_t & param )
{ 
  std::string f = "";
  bool exclude = true;

  if      ( param.has("include") ) { f = param.requires("include"); exclude = false; }
  else if ( param.has("exclude") ) f = param.requires("exclude"); 
  else Helper::halt( "need either include or exclude for MASK-FILE");
  
  if ( param.has( "intervals" ) )
    edf.timeline.load_interval_list_mask( f , exclude );
  else
    edf.timeline.load_mask( f , exclude );
}


// EPOCH-MASK  : based on epoch-annotations, apply mask

void proc_epoch_mask( edf_t & edf , param_t & param )
{
  
  // COMMAND NOT SUPPORTED

  std::set<std::string> vars;
  std::string onelabel;
  
  if ( param.has( "if" ) ) 
    {    
      if ( param.has( "ifnot" ) ) Helper::halt( "both if & ifnot specified" );
      vars = param.strset( "if" );
      onelabel = param.value("if");
      logger << " masking epochs that match " << onelabel << "\n";
    }
  else if ( param.has( "ifnot" ) ) 
    {
      vars = param.strset( "ifnot" );
      onelabel = param.value("ifnot");
      logger << " masking epochs that do not match " << onelabel << "\n";
    }
  else
    Helper::halt( "no if/ifnot specified" );
 
  edf.timeline.apply_simple_epoch_mask( vars , onelabel , param.has("if") );  

}

// EPOCH-ANNOT : directly apply epoch-level annotations from the command line
// with recodes

void proc_file_annot( edf_t & edf , param_t & param )
{ 
  
  std::string f = param.requires( "file" );

  std::vector<std::string> a;
  
  std::map<std::string,std::string> recodes;
  if ( param.has( "recode" ) )
    {
      std::vector<std::string> tok = Helper::quoted_parse( param.value( "recode" ) , "," );

      for (int i=0;i<tok.size();i++)
	{
	  std::vector<std::string> tok2 = Helper::quoted_parse( tok[i] , "=" );
	  if ( tok2.size() == 2 )
	    {
	      logger << "  remapping from " << tok2[0] << " to " << tok2[1] << "\n";
	      recodes[ Helper::unquote( tok2[0] ) ] = Helper::unquote( tok2[1] );
	    }
	  else
	    Helper::halt( "bad format for " + tok[i] );
	}
    }
  
  if ( ! Helper::fileExists( f ) ) Helper::halt( "could not find " + f );
  
  std::set<std::string> amap;

  std::ifstream IN1( f.c_str() , std::ios::in );
  while ( ! IN1.eof() )
    {
      std::string x;
      std::getline( IN1 , x );
      if ( IN1.eof() ) break;
      if ( x == "" ) continue;
      if ( recodes.find(x) != recodes.end() ) 
	{	  
	  x = recodes[x];
	}
      a.push_back( x );      
      amap.insert( x );
    }
  IN1.close();
  
  logger << " mapping " << amap.size() << " distinct epoch-annotations (" << a.size() << " in total) from " << f << "\n";


  //
  // Check correct number of epochs
  //
  
  if ( a.size() != edf.timeline.num_total_epochs() ) 
    Helper::halt( "epoch annotation file " + f + " contains " 
		  + Helper::int2str( (int)a.size() ) + " epochs but expecting " 
		  + Helper::int2str( edf.timeline.num_total_epochs() ) );


  //
  // create and store proper annotation events
  //
  
  annot_t::map_epoch_annotations( edf , 
				  a , 
				  f , 
				  edf.timeline.epoch_len_tp() , 
				  edf.timeline.epoch_increment_tp() );
  
}


// DUMP-MASK : output the current mask as an .annot file

void proc_dump_mask( edf_t & edf , param_t & param )
{
  if ( ! param.has("tag") )
    {
      edf.timeline.dumpmask();
      return;
    }

  // otherwise, create an ANNOT file from the MASK, i.e. for viewing
  std::string tag = param.requires( "tag" );
  std::string path = param.has( "path" ) ? param.value("path") : ".";
  edf.timeline.mask2annot( path, tag );
}





// COUNT-ANNOTS : show all annotations for the EDF

void proc_list_annots( edf_t & edf , param_t & param )
{
  summarize_annotations( edf , param );
}


// LIST-ANNOTS : list all annotations

void proc_list_all_annots( edf_t & edf , param_t & param )
{
  edf.timeline.list_all_annotations( param );
}

// TIME-TRACK : make EDF+

void proc_timetrack( edf_t & edf , param_t & param )
{
  edf.add_continuous_time_track();
}

// RESTRUCTURE : flush masked records

void proc_restructure( edf_t & edf , param_t & param )
{
  // just drop MASK'ed records, then reset mask
  edf.restructure( );
}


// DUMP-RECORDS : show all records (i.e. raw data)

void proc_record_dump( edf_t & edf , param_t & param )
{
  edf.add_continuous_time_track();  
  edf.record_dumper( param );
}


// STAGE : set and display sleep stage labels (verbose = F)
// HYPNO : verbose report on sleep STAGES     (verbose = F)

void proc_sleep_stage( edf_t & edf , param_t & param , bool verbose )
{
  
  std::string wake   = param.has( "wake" )  ? param.value("wake")  : "" ; 
  std::string nrem1  = param.has( "N1" ) ? param.value("N1") : "" ; 
  std::string nrem2  = param.has( "N2" ) ? param.value("N2") : "" ; 
  std::string nrem3  = param.has( "N3" ) ? param.value("N3") : "" ; 
  std::string nrem4  = param.has( "N4" ) ? param.value("N4") : "" ; 
  std::string rem    = param.has( "REM" )  ? param.value("REM")  : "" ; 
  std::string misc   = param.has( "?" )  ? param.value("?")  : "" ; 

  // either read these from a file, or display

  if ( param.has( "file" ) )
    {
      std::vector<std::string> ss = Helper::file2strvector( param.value( "file" ) );
      edf.timeline.hypnogram.construct( &edf.timeline , verbose , ss );
    }
  else
    {      
      edf.timeline.annotations.make_sleep_stage( wake , nrem1 , nrem2 , nrem3 , nrem4 , rem , misc );
      edf.timeline.hypnogram.construct( &edf.timeline , verbose );
    }

  // and output...
  edf.timeline.hypnogram.output( verbose );

}


// ED : compute 'electrical distance' measure of bridging

void proc_elec_distance( edf_t & edf , param_t & param )
{
  dsptools::elec_distance( edf , param );
}


// L1OUT : leave-one-out validation via interpolation of all signals
void proc_leave_one_out( edf_t & edf , param_t & param )
{
  dsptools::leave_one_out( edf , param );
}

// COH : calculate cross spectral coherence, using legacy code

void proc_coh_legacy( edf_t & edf , param_t & param )
{
  dsptools::coherence( edf , param , true );
}

// EMD : Empirical Mode Decomposition 
void proc_emd( edf_t & edf , param_t & param )
{
  dsptools::emd_wrapper( edf , param );
}

// ICA : fastICA on sample by channel matrix (whole trace)

void proc_ica( edf_t & edf , param_t & param )
{
  dsptools::ica_wrapper( edf , param );
}

// COH : calculate cross spectral coherence, using new/default code

void proc_coh( edf_t & edf , param_t & param )
{
  dsptools::coherence( edf , param );
}

// CORREL : correlation

void proc_correl( edf_t & edf , param_t & param )
{
  dsptools::correlate_channels( edf , param );
}

// MI : mutual information

void proc_mi( edf_t & edf , param_t & param )
{
  dsptools::compute_mi( edf , param );
}

// SPIKE : spike in a new bit of signal

void proc_spike( edf_t & edf , param_t & param )
{

  // create a new signal?
  std::string ns = "";
  
  if ( param.has( "new" ) ) ns = param.value( "new" );
  
  signal_list_t from_signal = edf.header.signal_list( param.requires( "from" ) );  
  signal_list_t to_signal   = edf.header.signal_list( param.requires( "to" ) );  
  
  if ( from_signal.size() != 1 ) Helper::halt( "no from={signal}" );
  if ( to_signal.size() != 1 ) Helper::halt( "no to={signal}" );
  
  int s1 = to_signal(0);
  int s2 = from_signal(0);
  
  double wgt = param.requires_dbl( "wgt" );

  spike_signal( edf , s1 , s2 , wgt , ns );
}

// PAC : phase amplitude coupling

void proc_pac( edf_t & edf , param_t & param )
{
  dsptools::pac( edf , param );
}

// CFC : generic cross-frequency coupling methods (other than PAC as above)

void proc_cfc( edf_t & edf , param_t & param )
{
  dsptools::cfc( edf , param );
}

// SUPPRESS-ECG : ECG supression

void proc_ecgsuppression( edf_t & edf , param_t & param )
{
  dsptools::ecgsuppression( edf , param );
}

void proc_bpm( edf_t & edf , param_t & param )
{
  dsptools::bpm( edf , param );
}


// COPY : mirror a signal
void proc_copy_signal( edf_t & edf , param_t & param )
{
  
  signal_list_t originals = edf.header.signal_list( param.requires( "sig" ) );

  std::string tag = param.requires( "tag" );
  
  for (int s=0;s<originals.size();s++)
    {

      if ( edf.header.is_data_channel( originals(s) ) )
	{

	  std::string new_label = originals.label( s ) + "_" + tag; 
	  
	  if ( ! edf.header.has_signal( new_label ) )
	    {
	      logger << " copying " << originals.label(s) << " to " << new_label << "\n";
	      edf.copy_signal( originals.label(s) , new_label );
	    }
	}
    }
}

// DROP : drop a signal

void proc_drop_signals( edf_t & edf , param_t & param )
{
  
  std::set<std::string> keeps, drops;
  if ( param.has( "keep" ) ) keeps = param.strset( "keep" );
  if ( param.has( "drop" ) ) drops = param.strset( "drop" );
  
  if ( param.has( "keep" ) && param.has( "drop" ) )
    Helper::halt( "can only specify keep or drop with SIGNALS" );
  
  if ( ! ( param.has( "keep" ) || param.has( "drop" ) ) ) 
    Helper::halt( "need to specify keep or drop with SIGNALS" );

  // if a keep list is specified, means we keep 
  if ( keeps.size() > 0 )
    {

      const int ns = edf.header.ns;

      for (int s = 0 ; s < ns ; s++ )
	{
	  std::string label = edf.header.label[s];

	  // is this signal on the keep list?
	  if ( keeps.find( label ) == keeps.end() )
	    {
	      
	      // or, does this signal have an alias that is on the keep list?
	      if ( cmd_t::label_aliases.find( label ) != cmd_t::label_aliases.end() )
		{
		  //std::cout << " has alias " << cmd_t::label_aliases[ label ]  << "\n";
		  if ( keeps.find( cmd_t::label_aliases[ label ] ) == keeps.end() )
		    {
		      //std::cout << "drps " << label << "\n";
		      drops.insert( label );
		      // OR ?? drops.insert( cmd_t::label_aliases[ label ] ) ; should be equiv.
		    }
		}
	      else
		drops.insert( label );
	    }
	  
	} // next signal
    }

  
  std::set<std::string>::const_iterator dd = drops.begin();
  while ( dd != drops.end() )
    {
      if ( edf.header.has_signal( *dd ) )
	{	  	  
	  int s = edf.header.signal( *dd );
	  //logger << "  dropping " << *dd << "\n";
	  edf.drop_signal( s );	  
	}
	++dd;
    }
  
}


// SLICE : pull out slices, based on 'file'

void proc_slice( edf_t & edf , param_t & param , int extract )
{
  
  // x = 1 (extract)
  // x = 0 (exclude)
  
  std::string filename = Helper::expand( param.requires( "file" ) );
  
  std::set<interval_t> intervals;

  if ( ! Helper::fileExists( filename ) ) Helper::halt( "could not find " + filename );


  std::ifstream IN1( filename.c_str() , std::ios::in );  
  while ( ! IN1.eof() )
    {
      interval_t interval;
      IN1 >> interval.start >> interval.stop;
      if ( IN1.eof() ) break;
      if ( interval.stop <= interval.start ) Helper::halt( "problem with interval line" );
      intervals.insert(interval);
    }
  IN1.close();

  logger << " read " << intervals.size() << " from " << filename << "\n";
  
  edf.slicer( intervals , param , extract );
  
}


// Reference tracks

void proc_reference( edf_t & edf , param_t & param )
{
  std::string refstr = param.requires( "ref" );
  signal_list_t references = edf.header.signal_list( refstr );
  
  std::string sigstr = param.requires( "sig" );
  signal_list_t signals = edf.header.signal_list( sigstr );

  edf.reference( signals , references );
  
}

// change record size for one or more signals

void proc_rerecord( edf_t & edf , param_t & param )
{
  double rs = param.requires_dbl( "dur" ); 

    logger << " altering record size from " << edf.header.record_duration << " to " <<  rs << " seconds\n";
  
    edf.reset_record_size( rs );

    logger << " now WRITE'ing EDF to disk, and will set 'problem' flag to skip to next EDF\n";

  proc_write( edf , param );
  globals::problem = true;
}

// uV or mV : set units for tracks

void proc_scale( edf_t & edf , param_t & param , const std::string & sc )
{
  std::string sigstr = param.requires( "sig" );
  signal_list_t signals = edf.header.signal_list( sigstr );
  const int ns = signals.size();  
  for (int s=0;s<ns;s++) 
    edf.rescale( signals(s) , sc );
}


// FLIP : change polarity of signal

void proc_flip( edf_t & edf , param_t & param  )
{
  std::string sigstr = param.requires( "sig" );
  signal_list_t signals = edf.header.signal_list( sigstr );
  const int ns = signals.size();  
  for (int s=0;s<ns;s++) 
    edf.flip( signals(s) );
}





//
// Helper functions
//
	      
// void attach_annot( edf_t & edf , const std::string & astr )
// {
  
//   // are we checking whether to add this file or no? 
  
//   if ( globals::specified_annots.size() > 0 && 
//        globals::specified_annots.find( astr ) == globals::specified_annots.end() ) return;

//   // otherwise, annotation is either 
  
//   // 1) in memory (and may or may not be in a file,
//   // i.e. just created)
  
//   annot_t * a = edf.timeline.annotations( astr );
  
//   // 2) or, not in memory but can be retrieved from a file
  
//   if ( a == NULL )
//     {
      
//       // search for file for any 'required' annotation
//       // (e.g. annot=stages,spindles)
      
//       std::string annot_file = edf.annotation_file( astr );
      
//       if ( annot_file == "" ) 
// 	{
// 	  logger << " no instances of annotation [" 
// 		 << astr << "] for " << edf.id << "\n";		  
// 	}
//       else
// 	{
// 	  // add to list and load in data (note, if XML, load() does nothing
// 	  // as all XML annotations are always added earlier
	  	  
// 	  bool okay = annot_t::load( annot_file , edf );

// 	  if ( ! okay ) Helper::halt( "problem loading " + annot_file );
	  		  
// 	}
//     }
// }


void cmd_t::parse_special( const std::string & tok0 , const std::string & tok1 )
{
  
  // add signals?
  if ( Helper::iequals( tok0 , "sig" ) )
    {		  
      std::vector<std::string> tok2 = Helper::quoted_parse( tok1 , "," );		        
      for (int s=0;s<tok2.size();s++) 
	cmd_t::signallist.insert(Helper::unquote(tok2[s]));		  
      return;
    }
  

  // default annot folder
  else if ( Helper::iequals( tok0 , "annot-folder" ) ||
	    Helper::iequals( tok0 , "annots-folder" ) ) 
    {
      if ( tok1[ tok1.size() - 1 ] != globals::folder_delimiter )
	globals::annot_folder = tok1 + globals::folder_delimiter ;
      else
	globals::annot_folder = tok1;		      
      return;
    }

  // not enforce epoch check for .eannot
  else if ( Helper::iequals( tok0 , "no-epoch-check" ) )
    {
      globals::enforce_epoch_check = false; 
      return;
    }

  // set default epoch length
  else if ( Helper::iequals( tok0 , "epoch-len" ) )
    {
      if ( ! Helper::str2int( tok1 , &globals::default_epoch_len ) )
	Helper::halt( "epoch-len requires integer value, e.g. epoch-len=10" );
      return;
    }


  // additional annot files to add from the command line
  // i.e. so we don't have to edit the sample-list
  else if ( Helper::iequals( tok0 , "annots-file" ) ||
	    Helper::iequals( tok0 , "annots-files" ) ||
	    Helper::iequals( tok0 , "annot-file" ) ||
	    Helper::iequals( tok0 , "annot-files" ) )
    {
      globals::annot_files = Helper::parse( tok1 , "," );
      return;
    }


  // specified annots (only load these)
  else if ( Helper::iequals( tok0 , "annots" ) || Helper::iequals( tok0 , "annot" ) ) 
    {
      param_t dummy;     
      dummy.add( "dummy" , tok1 );
      globals::specified_annots = dummy.strset( "dummy" , "," );      
      return;
    }
  
  // signal alias?
  if ( Helper::iequals( tok0 , "alias" ) )
    {
      cmd_t::signal_alias( tok1 );
      return;
    }
  
  // behaviour when a problem encountered
  if ( Helper::iequals( tok0 , "bail-on-fail" ) )
    {
      globals::bail_on_fail = Helper::yesno( tok1 );
      return;
    }
  
  // do not read EDF annotations
  if ( Helper::iequals( tok0 , "skip-annots" ) )
    {
      if ( tok1 == "1" || tok1 == "Y" || tok1 == "y" )
	globals::skip_edf_annots = true;
      return;
    }

  // do not read FTR files 
  if ( Helper::iequals( tok0 , "ftr" ) )
    {
      globals::read_ftr = Helper::yesno( tok1 );
      return;
    }

	
  // project path
  if ( Helper::iequals( tok0 , "path" ) )
    {
      globals::param.add( "path" , tok1 );
      return;
    }
  
  // do not force PM start-time
  if ( Helper::iequals( tok0 , "assume-pm-start" ) )
    {
      if ( tok1 == "0"
	   || Helper::iequals( tok1 , "n" )
	   || Helper::iequals( tok1 , "no" ) )
	globals::assume_pm_starttime = false; 
      return;
    }
  
  // signal alias?
  if ( Helper::iequals( tok0 , "alias" ) )
    {
      cmd_t::signal_alias( tok1 );
      return;
    }

  // power band defintions
  if ( Helper::iequals( tok0 , "slow" ) 
       || Helper::iequals( tok0 , "delta" ) 
       || Helper::iequals( tok0 , "theta" ) 
       || Helper::iequals( tok0 , "alpha" ) 
       || Helper::iequals( tok0 , "sigma" ) 
       || Helper::iequals( tok0 , "beta" ) 
       || Helper::iequals( tok0 , "gamma" ) 
       || Helper::iequals( tok0 , "total" ) ) 
    {
      std::vector<std::string> f = Helper::parse( tok1 , ",-" );
      if ( f.size() != 2 ) Helper::halt( "expecting band=lower,upper" );
      double f0, f1;
      if ( ! Helper::str2dbl( f[0] , &f0 ) ) Helper::halt( "expecting numeric for power range" );
      if ( ! Helper::str2dbl( f[1] , &f1 ) ) Helper::halt( "expecting numeric for power range" );
      if ( f0 >= f1 ) Helper::halt( "expecting band=lower,upper" );
      if ( f0 < 0 || f1 < 0 ) Helper::halt( "negative frequencies specified" );
      
      if      ( Helper::iequals( tok0 , "slow" ) )  globals::freq_band[ SLOW ] =  freq_range_t( f0 , f1 ) ;
      else if ( Helper::iequals( tok0 , "delta" ) ) globals::freq_band[ DELTA ] = freq_range_t( f0 , f1 ) ;
      else if ( Helper::iequals( tok0 , "theta" ) ) globals::freq_band[ THETA ] = freq_range_t( f0 , f1 ) ;
      else if ( Helper::iequals( tok0 , "alpha" ) ) globals::freq_band[ ALPHA ] = freq_range_t( f0 , f1 ) ;
      else if ( Helper::iequals( tok0 , "sigma" ) ) globals::freq_band[ SIGMA ] = freq_range_t( f0 , f1 ) ;
      else if ( Helper::iequals( tok0 , "beta" ) )  globals::freq_band[ BETA ] =  freq_range_t( f0 , f1 ) ;
      else if ( Helper::iequals( tok0 , "gamma" ) ) globals::freq_band[ GAMMA ] = freq_range_t( f0 , f1 ) ;
      else if ( Helper::iequals( tok0 , "total" ) ) globals::freq_band[ TOTAL ] = freq_range_t( f0 , f1 ) ;
      
    }

  // exclude individuals?
  if ( Helper::iequals( tok0 , "exclude" ) )
    {
      
      std::string xfile = Helper::expand( tok1 );
      
      if ( Helper::fileExists( xfile ) ) 
	{
	  std::ifstream XIN( xfile.c_str() , std::ios::in );
	  while ( !XIN.eof() ) 
	    {
	      // format: ID {white-space} any notes (ignored)
	      std::string line2;
	      std::getline( XIN , line2);
	      if ( XIN.eof() || line2 == "" ) continue;
	      std::vector<std::string> tok2 = Helper::parse( line2 , "\t " );
	      if ( tok2.size() == 0 ) continue;			      
	      std::string xid = tok2[0];
	      globals::excludes.insert( xid );
	    }
	  logger << "excluding " << globals::excludes.size() 
		 << " individuals from " << xfile << "\n";
	  XIN.close();
	}
      else logger << "**warning: exclude file " << xfile << " does not exist\n";

      return;
    }


  // not sure if/where this is used now
  
  if ( tok0[0] == '-' )
    {
      globals::param.add( tok0.substr(1) , tok1 );
      return;
    }

  
  // else a standard variable

  cmd_t::vars[ tok0 ] = tok1;

  
}




//
// Kludge... so that scope compiles on Mac... need to ensure the reduce_t is 
// called within the primary libluna.dylib, or else this function is not accessible
// when linking against libluna.dylib from scope...   TODO... remind myself 
// how to control which functions are exported, etc, in shared libraries
//

void tmp_includes()
{

  // dummy function to make sure reduce_t() [ which otherwise isn't used by luna-base ] is 
  // accessible from scope

  std::vector<double> d;
  std::vector<uint64_t> tp;
  uint64_t s1,s2;
  reduce_t r( &d,&tp,s1,s2,1);
  return;
}



//
// Make an EDF continuous
// 

void proc_continuous( edf_t & edf , param_t & param )
{
  logger << " forcing EDF to be continuous\n";
  edf.set_edf();
}
