<?php /*###################################################################*/
/*# hlmaps.inc                                                            #*/
/*#  - Function definitions for hlmaps routines                           #*/
/*#  - Copyright Brian Porter and Scott McCrory                           #*/
/*#  - Distributed under the GPL terms - see docs for info                #*/
/*#  - http://hlmaps.sourceforge.net                                      #*/
/*#                                                                       #*/
/*#########################################################################*/
/*# CVS $Id$
/*#########################################################################*/


/***************************************************************************/
/****                                                                   ****/
/****               # NO NEED TO EDIT BELOW THIS POINT #                ****/
/****                                                                   ****/
/***************************************************************************/




/***************************************************************************/
/* "GET" Subroutines                                                       */
/*  --These routines serve as the collection mechanism. They get data      */
/*    both from the config file and the data source (text file or mysql    */
/*    database) for use by the "PRINT" subroutines farther below.          */
/***************************************************************************/

/*-------------------------------------------------------------------------*/
function setDefaultValues()
// Set the default PHP parameters and variables if they were not specified
//	 for us in the URL or the .conf file. Do not edit these values-- modify
//	 your hlmaps.conf file instead. These are failsafe values here to fall
//	 back on.
{
		global $prefs;
		global $params;
			
		$prefs["LIST_TEMPLATE_FILE"]     = "/etc/hlmaps/list_template.html";
		$prefs["SINGLE_TEMPLATE_FILE"]	 = "/etc/hlmaps/single_template.html";
		
		$prefs["MAP_EXTENSION"]          = "bsp";
		$prefs["DOWNLOAD_EXT"]           = "zip";
		
		$prefs["SERVER"]                 = "127.0.0.1";
		$prefs["PORT"]                   = "27015";
		$prefs["PASS"]                   = "";

		$prefs["QSTAT_EXEC"]             = "/etc/qstat/sqtat";


		$prefs["DEFAULT_MAPS_PER_PAGE"]  = "20";

		$prefs["DATA_LOCATION"]          = "/etc/hlmaps/hlmaps.data";
		
		// Don't overwrite these values if they were supplied in hlmaps.php 
		if(!isset($prefs["MYSQL_SERVER"]))
			$prefs["MYSQL_SERVER"]       = "localhost";
		if(!isset($prefs["MYSQL_DATABASE"])) 
			$prefs["MYSQL_DATABASE"]     = "HLmaps";
		if(!isset($prefs["MYSQL_USER"])) 
			$prefs["MYSQL_USER"]         = "HLmaps";
		if(!isset($prefs["MYSQL_PASSWORD"])) 
			$prefs["MYSQL_PASSWORD"]     = "HLmaps";
			
		$prefs["MYSQL_TABLE"]            = "maps";
		$prefs["MYSQL_MAPNAME_FIELD"]    = "map";
		$prefs["MYSQL_IMAGEURL_FIELD"]   = "imageurl";
		$prefs["MYSQL_DOWNLOAD_FIELD"]   = "downloadurl";
		$prefs["MYSQL_TEXTFILE_FIELD"]   = "textfile";
		$prefs["MYSQL_POPULARITY_FIELD"] = "popularity";
		$prefs["MYSQL_MAPCYCLE_FIELD"]   = "mapcycle";
		$prefs["MYSQL_SIZE_FIELD"]       = "size";
		$prefs["MYSQL_MODDATE_FIELD"]    = "moddate";

		// Don't overwrite these values if they were supplied in hlmaps.php 
		if(!isset($prefs["MYSQL_PREFS_TABLE"]))
			$prefs["MYSQL_PREFS_TABLE"]  = "prefs";
		if(!isset($prefs["MYSQL_PREFS_VALFIELD"])) 
			$prefs["MYSQL_PREFS_VALFIELD"] = "prefvalues";
		if(!isset($prefs["MYSQL_PREFS_KEYFIELD"])) 
			$prefs["MYSQL_PREFS_KEYFIELD"] = "prefkeys";

		$params["mode"]                  = "list";
		$params["mapcount"]              = $prefs["DEFAULT_MAPS_PER_PAGE"];
		$params["page"]                  = "1";
		$params["sort"]                  = "map";
		$params["order"]                 = "a";
		$params["mapname"]               = "";
}

/*-------------------------------------------------------------------------*/
function getPreferences()
// Opens the config file specified by the $CONFIG_FILE variable at the top
//	 of this file and imports the settings into the $prefs array.
{
 global $CONFIG_FILE;
 global $prefs;

 if($myText = @file($CONFIG_FILE))  // the file opened okay (suppress warnings if not)
 {
	 while(list($index, $aryVal) = each($myText))  // take the next line from the file & put it in $aryVal
	 {
		 // parse the line, return the variable name and it's value
		 list($myKey, $myVal) = each(parseConfigLine($aryVal)); 
		 $prefs[$myKey] = $myVal;  // set the value
	 }
	 return(true); // success
 }
 else // couldn't open the file-- try mysql server
 {	 
	 $mysql_port = "3306";
	 $mysql_query = "SELECT * FROM " . $prefs["MYSQL_PREFS_TABLE"];
	
	 if($connection = mysql_connect($prefs["MYSQL_SERVER"] . ":" . $mysql_port, 
									$prefs["MYSQL_USER"], $prefs["MYSQL_PASSWORD"]))
	 {
		 if(mysql_select_db( $prefs["MYSQL_DATABASE"], $connection) 
										&& $result = mysql_query($mysql_query) )
		 {
			 while($result_row = mysql_fetch_assoc($result))
			 {
				 $tempKey = $result_row[$prefs["MYSQL_PREFS_KEYFIELD"]];
				 $tempVal = $result_row[$prefs["MYSQL_PREFS_VALFIELD"]];
				 
				  // Protect our already set MySQL prefs from being overwritten 
				  //   by those in the .cron's .conf file! 
				 if( strcmp($tempKey, "MYSQL_SERVER") &&
				     strcmp($tempKey, "MYSQL_USER") &&
				     strcmp($tempKey, "MYSQL_PASSWORD") &&
				     strcmp($tempKey, "MYSQL_DATABASE") &&
				     strcmp($tempKey, "MYSQL_PREFS_TABLE") && 
				     strcmp($tempKey, "MYSQL_PREFS_KEYFIELD") &&
				     strcmp($tempKey, "MYSQL_PREFS_VALFIELD")    )
				 {
				   $prefs[$tempKey] = $tempVal;
				 }
			 }
		 }
		 else
		 {
			 return(false);
		 }
		 mysql_close();
		 return(true);
	 }
	 else
	 {
		 return(false);
	 }
	 //return(false); // could not open the config file
 }
 		print("getPreferences() couldn't open a config file, OR connect to the mysql database!<br>\n");
}

/*-------------------------------------------------------------------------*/
function getParameters()
// Reads from the query string passed to hlmaps.php in the URL and stores the 
//	 values in the $params array for use in determining the following
//	 traits: 
//			--'mode' = [non-existent] || 'list' || 'single'
//          --'search' = [non-existent] || [search string]
//			--'map' = ["short name" of any map] (this field is *required* if mode=single)
//			--'mapcount'= [non-existent] || [a digit]
//			--'page' = [non-existent] || [a digit less than: number of maps divided by mapcount]
//			--'sort' = [non-existent] || 'map' || 'image' || 'popularity'
//									|| 'mapcycle' || 'size' || 'moddate' || 'download'
//			--'order' = [non-existent] || 'a' || 'd'
//
// I.e.-- The URL "hlmaps.php?mode=list&page=1&mapcount=25&sort=map&order=a"
//					 will result in a list view of page 1 with the first 25 maps on
//					 it, sorted in ascending order by map name.
//				The URL "hlmaps.php?mode=single&map=de_dust" will result in a 
//					 page detailing the map named "de_dust".
//              The URL "hlmaps.php?search=de_" will result in a listing containing
//                   all maps that contain "de_".
{
 global $params;

 // These variables are defined by PHP each time the script is run on a 
 // 	webserver. Each of these represent a variable passed to the script 
 // 	as part of a query string in the URL used to access the page.
 global $mode;
 global $search;
 global $mapcount;
 global $page;
 global $sort;
 global $order;
 global $map;
 
 // Make sure the query string contained the variable before trying to set it.
 if(isset($mode)){     $params["mode"] = $mode;         }
 if(isset($search)){   $params["search"] = $search;     }
 if(isset($mapcount)){ $params["mapcount"] = $mapcount; }
 if(isset($page)){     $params["page"] = $page;         }
 if(isset($sort)){     $params["sort"] = $sort;         }
 if(isset($order)){    $params["order"] = $order;       }
 if(isset($map)){      $params["map"] = $map;             // If a map name is set,
                       $params["mode"] = "single";      } //   auto set to single view.
}

/*-------------------------------------------------------------------------*/
function getMapDetails_text()
// Reads from the DATA_LOCATION set in the config file if using a text file
//	 as a data source. Each line is read in and a new map record is constructed
//	 and placed in $allMaps for later use.
{
 global $prefs;
 global $allMaps;
 global $numOfMaps;

 $data = fopen($prefs["DATA_LOCATION"], "r");
 
 if($data) // the file was opened successfully
 {
	 $map = new mapclass;
	 while(list($map->map, $map->textfile, $map->downloadurl, $map->imageurl, 
                $map->popularity, $map->mapcycle, $map->size, $map->moddate)
                                                         = fgetcsv($data, 200))
	 {
		 array_push($allMaps, $map);
		 $numOfMaps++;
	 }
	 
	 fclose($data);
	 return(true);
 }
 else
 {
	 return(false);
 }
}

/*-------------------------------------------------------------------------*/
function getMapDetails_mysql()
// Reads map data from a MySQL database if DATA_LOCATION is set to "mysql"
//	 in the config file. A new map record is created from each query, and
//	 added to $allMaps for later use.
{
 global $prefs;
 global $allMaps;
 global $numOfMaps;
 
 $mysql_port = "3306";
 $mysql_query = "SELECT * FROM " . $prefs["MYSQL_TABLE"];
 $tempMap = new mapclass;

 if($connection = mysql_connect($prefs["MYSQL_SERVER"] . ":" . $mysql_port, 
                                $prefs["MYSQL_USER"], $prefs["MYSQL_PASSWORD"]))
 {
	 if(mysql_select_db($prefs["MYSQL_DATABASE"], $connection) 
        && $result = mysql_query($mysql_query))
	 {
		 while($result_row = mysql_fetch_assoc($result))
		 {
			 $tempMap->map = $result_row[$prefs["MYSQL_MAPNAME_FIELD"]];
			 $tempMap->textfile = $result_row[$prefs["MYSQL_TEXTFILE_FIELD"]];
			 $tempMap->downloadurl = $result_row[$prefs["MYSQL_DOWNLOAD_FIELD"]];
			 $tempMap->imageurl = $result_row[$prefs["MYSQL_IMAGEURL_FIELD"]];
			 $tempMap->popularity = $result_row[$prefs["MYSQL_POPULARITY_FIELD"]];
			 $tempMap->mapcycle = $result_row[$prefs["MYSQL_MAPCYCLE_FIELD"]];
			 $tempMap->size = $result_row[$prefs["MYSQL_SIZE_FIELD"]];
			 $tempMap->moddate = $result_row[$prefs["MYSQL_MODDATE_FIELD"]];
			 
			 array_push($allMaps, $tempMap);
			 $numOfMaps++;
		 }
	 }
	 else
	 {
		 return(false);
	 }
	 mysql_close();
	 return(true);
 }
 else
 {
	 return(false);
 }
}

/*-------------------------------------------------------------------------*/
function getRealtimeServerStatus()
// Gathers information from the game server to display a token next to the
//	 map currently in play. Return true if successful, false on failure. Much
//	 more information is available than what is currently used by the script.
//	 Use the "DEBUG OUTPUT" at the end of the  function to get an idea of
//	 what else can be used.
/* 
Raw qstat data formats: 
 --General server info-- (element 0 of $command_output)
		[0] protocol version, 
		[1] command-line arg (IP address or host name), 
		[2] server name, 
		[3] map name, 
		[4] maximum players, 
		[5] current players, 
		[6] average response time, 
		[7] number of retries 
		
 --Server rules-- (element 1 of $command_output)
		(unique to each half-life mod)
		
 --Player info-- (elements 2+ of $command_output)
		player name, 
		frags, 
		connect time (in seconds)
*/
{
 global $serverInfo;
 global $serverRules;
 global $playerList;
 global $prefs;

 // Leave the following variables out of the preferences-- we don't want
 // 	end users messing around with these-- only developers.
 $QSTAT_OPTIONS = " -P -R -nh -raw "; // show Player data, Server rules, 
                                      // No Header line, all in RAW format.
 $QSTAT_RAWDELIM = ",";               // Delimits data fields in the raw output.
 $QSTAT_SERVERTYPE = "hls";           // Tells qstat this is a Half-Life server.
 
 // Compile the command line to call qstat
 $command_line = $prefs["QSTAT_EXEC"] . $QSTAT_OPTIONS . $QSTAT_RAWDELIM 
								 . " -" . $QSTAT_SERVERTYPE . " " . $prefs["SERVER"] . ":" 
								. $prefs["PORT"];

 // Call qstat, trap output and return value, and suppress warnings if 
 // 	the executable can not be found or run
 @exec($command_line, $command_output, $return_value);
 
 if(!$return_value && !ereg("TIMEOUT", $command_output[0])) // anything but 0 means an error code
 { 
	 // Break the first line of the output into a general server info array
	 $serverInfo = explode($QSTAT_RAWDELIM, $command_output[0]);
	 
	 // Make the array associative instead of indexed
	 $serverInfo["protocol"] = $serverInfo[0];
	 $serverInfo["ip_host"] = $serverInfo[1];
	 $serverInfo["name"] = $serverInfo[2];
	 $serverInfo["map"] = $serverInfo[3];
	 $serverInfo["max"] = $serverInfo[4];
	 $serverInfo["current"] = $serverInfo[5];
	 $serverInfo["response"] = $serverInfo[6];
	 $serverInfo["retries"] = $serverInfo[7];
	 $serverInfo = array_splice($serverInfo, 8, 7); // Cut out the indexed elements
	 
	 // Break the second line into an array of raw rule=value elements
	 $rawRules = explode($QSTAT_RAWDELIM, $command_output[1]);
	 // ...then break each element into a [key] => value pair in the finished array
	 while(list($k, $v) = each($rawRules) )
	 {
		 list($rule, $value) = split("=", $v);
		 $serverRules[$rule] = $value;
	 }
	 
	 // Take all elements after server info as player listings
	 $players = array_splice($command_output, 2);
	 $thisPlayer = new playerClass;
	
	 // Populate the global playerList with the current data
	 while((list($k, $playerString) = each($players)) && $playerString)
	 {
		 $temp = explode($QSTAT_RAWDELIM, $playerString);
		 $thisPlayer->name = $temp[0];
		 $thisPlayer->frags = $temp[1];
		 $thisPlayer->time = $temp[2];
		 if($temp[0]){ array_push($playerList, $thisPlayer); }
	 }
	 
 }
 else
 {
	 return(false);
 }
		 
 //#*#* DEBUG OUTPUT *
 /*print("\n<br> Command used line is: " . $command_line);
 print("\n<br> Return value from qstat is: " . $return_value);
 print("\n<br> Command output from qstat is: ");
 print_r($command_output);
 print("\n<br><br> General Server Info is: ");
 while(list($k,$v) = each($serverInfo)){print("\n<br>" . $k . " = " . $v);}
 print("\n<br><br>Server Rules are: ");
 while(list($k,$v) = each($serverRules)){print("\n<br>" . $k . " = " . $v);}
 print("\n<br><br>Players on the server are: ");
 while(list($k,$v) = each($playerList)){
	 print("\n<br>" . $k . " = Name- " . $v->name . ", Frags- " . $v->frags 
				 . ", Time Connected- " . (int)($v->time / 60) . " minutes");}
 // END DEBUG OUTPUT */
 
 return(true);
}

/***************************************************************************/
/* "OTHER" Subroutines                                                     */
/* --These routines are neither GET or PRINT procedures, they provide      */
/*   support for both of these groups.                                     */
/***************************************************************************/

/*-------------------------------------------------------------------------*/
function printAllVars()
//#*#* DEBUG function to "watch" most of the values used in the script
{
	global $prefs;
	global $params;
	global $allMaps;
	global $displayedMaps;
	global $numOfMaps;
	
	// print all preferences
	print("<h3>Preferences</h3>\n");
	while(list($k, $v) = each($prefs))
	{
		print("\"" . $k . "\" = " . $v . "<br>\n");
	}
	
	// print all parameters from url query line
	print("<h3>Parameters</h3>\n");
	while(list($k, $v) = each($params))
	{
		print("\"" . $k . "\" = " . $v . "<br>\n");
	}
	
	// print all map records
	print("<h3>All Maps</h3>\n");
	$temp = new mapclass;
	while(list($k, $temp) = each($allMaps))
	{
		print("\"" . $k . "\" = " . $temp->map . ", " . $temp->imageurl . ", "
              . $temp->downloadurl . ", " . $temp->textfile . ", " . $temp->popularity . ", "
              . $temp->mapcycle . ", " . $temp->size . ", " . $temp->moddate . ", " . "<br>\n");
	}
	
	// print displayed maps indices
	print("<h3>Displayed Maps</h3>\n");
	while(list($k, $v) = each($displayedMaps))
	{
		print("\"" . $k . "\" = " . $v . "<br>\n");
	}

	print("<h3>Number of Maps</h3>\n");
	print("\"numOfMaps\" = " . $numOfMaps);
	
	print("<h3>Functions Defined</h3>\n");
	$funcs = get_defined_functions();
	print_r($funcs['user']);
	
	print("<h3>Included Files</h3>\n");
	print_r(get_included_files());
	//print_r(get_required_files());

	print("<h3>Globals</h3>\n");
	while(list($k, $v) = each($GLOBALS))
	{ print($k . " = " . $v . "<br>\n"); }
	
}

/*-------------------------------------------------------------------------*/
function parseConfigLine($configStr)
// Converts a raw string read from the config file into a variable/value 
//	 pair, and returns them as an associative array.
{
	$configStr = trim($configStr);	// removes leading and trailing whitespace
	$configStr = preg_replace("/#.*$/", "", $configStr); // remove comments
	$configStr = preg_replace("/\s+/", " ", $configStr); // remove multiple spaces

	if(preg_match("/ = /", $configStr)) // have a variable declaration WITH A VALUE
	{
		list($k, $v) = split(" = ", $configStr); // split the string on the "=" into vars
	}
	else // it's a left over blank line
	{
		$k = "blank"; // route to a dummy location
		$v = "";			// ...with no value
	}

	return(array($k=>$v)); // return an array with the key/value pair
}

/*-------------------------------------------------------------------------*/
function compare($a, $b)
// Compares $a and $b. Return 1 if $a is greater than $b, -1 if $b is greater,
//	 and 0 if they are equal.
{
	if($a == $b){ return(0); }
	elseif($a > $b){ return(1); }
	else{ return(-1); }
}

/*-------------------------------------------------------------------------*/
function mapSort($a, $b)
// Used exclusively by populateMapsForPage(). This is the sorting function
//	 used by usort() to sort the maps by each field available. The function
//	 returns -1 if the value of $a is "less than" that of $b and 1 if $a is 
//	 "greater than" $b. If they are equal, the function sorts on a secondary
//	 basis by map name. (This makes the map list looks nice and orderly.)
{
	global $params;
	
	switch($params["sort"])
	{
		case "image": 
					 $result = strnatcasecmp($a->imageurl, $b->imageurl); 
					 break;
								
		case "popularity": 
					 $result = compare($a->popularity, $b->popularity); 
					 break; 	
						
		case "mapcycle": 
					 $result = compare($a->mapcycle, $b->mapcycle); 
					 break; 
							
		case "size": 
					 $result = compare($a->size, $b->size); 
					 break; 	
						
		case "moddate": 
					 $result = compare($a->moddate, $b->moddate); 
					 break; 	 
					 
		case "download":
					 $result = strnatcasecmp($a->downloadurl, $b->downloadurl);
					 break; 	 
					 
		case "map": // map IS the default case, so continue on down...
		
		default: // just set $result to 0, and let the if() below sort for us
					 $result = 0; 
					 break;  
	}
	// our values were equal, so let's sort among those by name instead
	if($result == 0)
	{ 
		$result = strnatcasecmp($a->map, $b->map); 
	} 
	return($result);	 // return what's left
}

/*-------------------------------------------------------------------------*/
function filterMapsBySearch($mapArray, $searchStr)
// Searches $mapArray for elements that have a map name containing 
//   $searchStr. Returns an array stripped of elements that do not match
//   the criteria.
{
  global $numOfMaps;
  
  $filteredArray = array();
  $numOfMaps = 0;
  
  while( list($k, $v) = each($mapArray) )
  {
    //echo "$k";  //#*#* DEBUG
  	if( strstr($v->map, $searchStr) )
  	{
      //echo " found match";  //#*#* DEBUG
  		$numOfMaps++;
  	  $filteredArray[] = $v;
    }
    //echo ", ";  //#*#* DEBUG
  }

  return $filteredArray;
}


/*-------------------------------------------------------------------------*/
function populateMapsForPage()
// Grabs only the map records we need for the current display mode and 
//	 stores them in an array of index references for use by the 
//	 "printThisMap_..." functions.
{
	global $params;
	global $allMaps;
	global $filteredMaps;
	global $displayedMaps;
	global $numOfMaps;
	
	reset($allMaps);	// make sure we're at the start of the array
	
  
  // Search function section
  if(isset($params["search"]))  // A search string is present...
  {
    //echo " calling search filter ";  //#*#* DEBUG
    $filteredMaps = filterMapsBySearch($allMaps, $params["search"]);  // ..so remove anything that doesn't match
    if(count($filteredMaps) == 1)
      $params["mode"] = "single";
  }
  else  // No search string, just pass $allMaps along.
  {
    //echo "no search filter";  //#*#* DEBUG
    $filteredMaps = $allMaps;
  }

  //#*#* DEBUG
  /*print(" printing found maps ");
  while( list($k, $v) = each($filteredMaps) )
  {
    print($v->map . "<br />\n");
  } */


  // Generate a list of indices (one or multiple)
  if($params["mode"] == "single")  // grab index of map we want to display
	{
    //echo " in single function ";  //#*#* DEBUG
		$tempMap = new mapclass;
		
		while(list($i, $tempMap) = each($filteredMaps))
		{
			if($tempMap->map == $params["map"])
			{
				$displayedMaps[] = $i;
				return(true);  // indicates success
			}
		}
		return(false);	// indicates function failed to find the map
	}
	else	// determine criteria and make a list of indices
	{
    //echo " in list function ";  //#*#* DEBUG
		usort($filteredMaps, "mapSort");  // sort the maps by the url parameter "sort"
		
		$start = ($params["page"] - 1) * $params["mapcount"];  // figure the beginning index for this page of maps
		$end = $params["page"] * $params["mapcount"];  // figure the ending index
		
		if($params["order"] == "a")  // we sort in ascending order
		{
			// we've gone past the end of the array, so stop at $numOfMaps instead
			if($end > $numOfMaps)
			{ 
				$end = $numOfMaps; 
			}
			for($i = $start; $i < $end; $displayedMaps[] = $i++);  // add the indices to the displayedMaps array
		}
		else	// we start from the end of the list
		{
			$start = $numOfMaps - $start - 1;  // so flip-flop the start index to be in relation to the LAST element
			$end = $numOfMaps - $end - 1;  // and the same for the ending index
			
			// we've gone past the beginning of the array, so stop at 0
			if($end < 0)
			{ 
				$end = 0; 
			}
			for($i = $start; $i >= $end; $displayedMaps[] = $i--);	// add the indices to the displayedMaps array
		}
		return(true);
	}
}

/*-------------------------------------------------------------------------*/
function popNextMap()
// Grabs the last value from the array $displayedMaps and uses it as an
//	 index for $allMaps in order to set $thisMap to the current map to be
//	 displayed.
{
	global $thisMap;
	global $filteredMaps;
	global $displayedMaps;
	
	// suppress warnings from the array and return zero instead
	if($thisMap = @$filteredMaps[array_shift($displayedMaps)])
	{ 
		return(true); 
	}
	else
	{ 
		return(false); 
	}
}
	
/*-------------------------------------------------------------------------*/
function mysqlBailOut()
{

}

/*-------------------------------------------------------------------------*/
function report_error($file, $line, $message)
// Handy little function to have around. Call it with the following 
//		commented line. Prints the current file name and line number.
//	 (Same as the built-in warnings and errors)
// TO USE: report_error(__FILE__, __LINE__, "Something went wrong!");
{
	echo "An error occured in $file on line $line: $message.";
}


/***************************************************************************/
/* "PRINT" Subroutines                                                     */
/* --These routines can be called from an HTML template page. Comments     */
/*   on output format and usage are included below each function's         */
/*   declaration.                                                          */
/***************************************************************************/

/*-------------------------------------------------------------------------*/
function printTotalMapCount()
// Prints the total number of maps found on the server as a digit
{
	global $numOfMaps;
	print($numOfMaps);	
}

/*-------------------------------------------------------------------------*/
function printCredits()
// Prints text citing hlmaps version, homepage, and contact information. This
//	 fuction is required to exist on every hlmaps page, and will be displayed
//	 automatically if not included in a template.
{
	static $printed;
	global $VERSION;
	global $AUTHOR_NAME;
	global $AUTHOR_EMAIL;
	global $HOME_PAGE;
	global $SCRIPT_NAME;
	global $PACKAGE_NAME;
	
	if(!isset($printed)){ $printed = 0; }
	if(isset($printed) && !$printed)
	{
		$printed = true;
		print("Generated by <a class=creditsLinkClass href=\"" . $HOME_PAGE 
              . "\"><b>" . basename($PACKAGE_NAME) . "</b></a> " . $VERSION
              . "<br>" . "PHP version by <a class=creditsLinkClass href=\""
              . $AUTHOR_EMAIL . "\">" . $AUTHOR_NAME . "</a>" 	 );
	}
}

	
/*-------------------------------------------------------------------------*/
function printMapsPerPageWidget($prompt = "Maps per page: ")
// Prints an HTML form that allows that user to select how many maps
//	 to display per page. The function accepts an option argument
//	 for the text to display in front of the widget describing it's
//	 function. The default (if no string is passed) is "Maps per page: "
//		 ...good luck understanding the print()! (It was created in GoLive
//				and imported here. Look at all those backslashes!)
{
	global $prefs;
	global $params;
	global $SCRIPT_NAME;
	print("<form class=mapsPerPageFormClass method=\"post\" action=\"". $SCRIPT_NAME ."\""
				. " enctype=\"application/x-www-form-urlencoded\"><center>\n" . $prompt
				. "<select name=\"mapcount\">\n" . "<option value=\"" 
				. $prefs["DEFAULT_MAPS_PER_PAGE"] . "\">" 
				. $prefs["DEFAULT_MAPS_PER_PAGE"] ."</option>\n"
				. "<option value=\"10\">10</option>\n"
				. "<option value=\"20\">20</option>\n"
				. "<option value=\"50\">50</option>\n"
				. "<option value=\"100\">100</option>\n" . "</select>\n"
				. "<input type=\"hidden\" value=\"" . $params["mode"] . "\" name=\"mode\">"
				. "<input type=\"hidden\" value=\"" . $params["sort"] . "\" name=\"sort\">"
				. "<input type=\"hidden\" value=\"" . $params["order"] . "\" name=\"order\">"
				. "<input type=\"submit\" value=\"Update\" /></center>\n"
				. "</form>");
}

/*-------------------------------------------------------------------------*/
function printFullOptionsForm()  /*** DEPRECIATED ***/
// This function is not internationally friendly as it does not accept
//	 arguments to customize it's output language. It is therefore depreciated,
//	 and included only for example purposes. It outputs an html form with
//	 all of hlmaps' options available. This function only works in a "list" view.
{
	global $prefs;
	global $SCRIPT_NAME;
	print("<form class=mapsPerPageFormClass name=\"Options\" action=\"" 
			. $SCRIPT_NAME . "\" method=\"get\" enctype=\"application/x-www-form-urlencoded\">\n"
			. "Sort <select name=\"mapcount\" size=\"1\">\n"
				. "<option value=\"" . $prefs["DEFAULT_MAPS_PER_PAGE"] . "\" selected>" 
				. $prefs["DEFAULT_MAPS_PER_PAGE"] . "</option>\n"
				. "<option value=\"10\">10</option>\n"
				. "<option value=\"20\">20</option>\n"
				. "<option value=\"50\">50</option>\n"
				. "<option value=\"100\">100</option>\n"
			. "</select> maps per page by <select name=\"sort\" size=\"1\">\n"
				. "<option value=\"map\" selected>name</option>\n"
				. "<option value=\"image\">snapshot</option>\n"
				. "<option value=\"popularity\">popularity</option>\n"
				. "<option value=\"mapcycle\">place in mapcycle</option>\n"
				. "<option value=\"size\">map size</option>\n"
				. "<option value=\"moddate\">last modified date</option>\n"
				. "<option value=\"download\">downloads available</option>\n"
			. "</select> in <select name=\"order\" size=\"1\">\n"
				. "<option value=\"a\" selected>ascending</option>\n"
				. "<option value=\"d\">descending</option>\n"
			. "</select> order.\n"
			. "<p><input type=\"hidden\" value=\"list\" name=\"view\">\n"
				 . "<input type=\"hidden\" value=\"1\" name=\"page\"></p>\n"
			. "<p><input type=\"submit\" value=\"Update\"></p>\n"
		. "</form>");
}
	
/*-------------------------------------------------------------------------*/
function printSearchForm($prompt = "Enter a map name")
// This function will eventually allow a user to search for a map. On an
//	 exact match, a single map page will be displayed. If thee are no exact
//	 matchs, the closest fits from those maps on the server will be 
//	 displayed in a list view.
{
	global $prefs;
	global $params;
	global $SCRIPT_NAME;

	print("<form name=\"Search\" action=" . $SCRIPT_NAME
				. " method=\"get\" enctype=\"application/x-www-form-urlencoded\">\n"
				. "<input type=\"text\" name=\"search\" size=\"24\""
				. " maxlength=\"60\" value=\"");
  
//  if(isset($params["search"]))
  if(TRUE)
    print($params["search"]);
  else
    print($prompt);

  print("\">\n" . "<input type=\"submit\" value=\"Search\">\n" .
        "<input type=\"reset\" value=\"Clear\">\n" . "</form>");
}

/*-------------------------------------------------------------------------*/
function printPageSelectors($prompt_next = "Next", $prompt_prev = "Prev")
// Prints an HTML table that allows the user to navigate the multiple
//	 pages it may take to display all the maps.
{
	global $params;
	global $numOfMaps;
	global $SCRIPT_NAME;
	
	$div = $numOfMaps / $params["mapcount"];
	if($div < 1)
	{ 
		$div = 1; 
	}
	
	if($numOfMaps % $params["mapcount"] == 0 || $div == 1)
	{
		$numOfPages = $div; 
	}
	else
	{
		$numOfPages = (int)$div + 1; 
	}
	
	// start a table
	print("<table><tr>");

	// print a "previous" button if applicable
	if($params["page"] > 1)
	{
		print("<td class=pageSelectorsClass><a href=\"" . $SCRIPT_NAME . "?mode=list&sort=" . $params["sort"] 
					. "&order=" . $params["order"] . "&mapcount=" . $params["mapcount"] 
					. "&page=" . ($params["page"] - 1) . "\">" . $prompt_prev . "</a></td>\n");
	}

	// print a button for each page of maps
	if($numOfPages > 1)
	{
		for($i = 1; $i <= $numOfPages; $i++)
		{
			if($i == $params["page"])
			{
				print("<th class=pageSelectorsClass>");
			}
			else
			{
				print("<td class=pageSelectorsClass>");
			}
			print("<a href=\"" . $SCRIPT_NAME . "?mode=list&sort=" . $params["sort"] 
						. "&order=" . $params["order"] . "&mapcount=" . $params["mapcount"] 
						. "&page=" . $i . "\">" . $i . "</a></td>\n");
		}
	}

	// print a "next" button if applicable
	if($params["page"] > 0 && $params["page"] < $numOfPages)
	{
		print("<td class=pageSelectorsClass><a href=\"" . $SCRIPT_NAME . "?mode=list&sort=" . $params["sort"] 
					. "&order=" . $params["order"] . "&mapcount=" . $params["mapcount"] 
					. "&page=" . ($params["page"] + 1) . "\">" . $prompt_next . "</a></td>\n");
	}

	// end the table
	print("</tr></table>");
}

/*-------------------------------------------------------------------------*/
function printListHeading($sortType, $prompt)
// This is a template function designed to reduce duplicate code. All of 
//	 the "printListHeading" functions below call this function with two
//	 arguments: a string representing the sort type for the hyperlink being
//	 created (such as "map" or "moddate") and the prompt passed by the
//	 user (or the defualt) to be displayed as the text for the hyperlink.
//	 The function returns a string containing the completed HTML hyperlink.
//	 This function should NOT be called from an HTML template page directly.
//	 Use the specific functions below instead.
{
	global $SCRIPT_NAME;
	global $params;
	$myString = "<a href=\"" . $SCRIPT_NAME . "?mode=list&sort=" . $sortType . "&order=";
	if($params["sort"] == $sortType && $params["order"] == "a")
	{
		$myString .= "d";
	}
	else
	{
		$myString .= "a";
	}
	$myString .= "&mapcount=" . $params["mapcount"] . "&page=" . $params["page"] . "\">";
	if($params["sort"] == $sortType)
	{
		$myString .= "<font class=higlightedClass>" . $prompt . "</font>";
	}
	else
	{
		$myString .= $prompt;
	}
	$myString .= "</a>";
	return $myString;
}

/*-------------------------------------------------------------------------*/
function printListHeading_name($prompt = "Name")
// Prints an HTML hyperlink that will sort the map list by name when clicked. 
//	 The argument become the text of the hyperlink, and is optional. The 
//	 default value will be used if none is supplied when calling the function.
{
	print(printListHeading("map", $prompt));
}


/*-------------------------------------------------------------------------*/
function printListHeading_image($prompt = "Image")
// SEE DESCRIPTION FOR printListHeading_name()
{
	print(printListHeading("image", $prompt));
}

/*-------------------------------------------------------------------------*/
function printListHeading_popularity($prompt = "Popularity")
// SEE DESCRIPTION FOR printListHeading_name()
{
	print(printListHeading("popularity", $prompt));
}

/*-------------------------------------------------------------------------*/
function printListHeading_mapcycle($prompt = "In mapcycle?")
// SEE DESCRIPTION FOR printListHeading_name()
{
	print(printListHeading("mapcycle", $prompt));
}

/*-------------------------------------------------------------------------*/
function printListHeading_size($prompt = "Map Size")
// SEE DESCRIPTION FOR printListHeading_name()
{
	print(printListHeading("size", $prompt));
}

/*-------------------------------------------------------------------------*/
function printListHeading_modDate($prompt = "Mod Date")
// SEE DESCRIPTION FOR printListHeading_name()
{
	print(printListHeading("moddate", $prompt));
}

/*-------------------------------------------------------------------------*/
function printListHeading_download($prompt = "Download Available")
// SEE DESCRIPTION FOR printListHeading_name()
{
	print(printListHeading("download", $prompt));
}


/*-------------------------------------------------------------------------*/
function printThisMap_Name($currently_in_play = "Map currently in play: ")
// Prints HTML for the map's name. Creates a hyperlink either to a single
//	 map view mode, or back to the last list view mode. $prompt is the text
//	 that is displayed if the map is current active on the server.
{
	global $prefs;
	global $params;
	global $thisMap;
	global $serverInfo;
	global $SCRIPT_NAME;
	if($params["mode"] == "list") // then link to a single map page
	{
		print("<a href=\"" . $SCRIPT_NAME . "?mode=single&map=" . $thisMap->map 
	          . "\">" . $thisMap->map . "</a>");
	}
	else // then link to a list view
	{
		print("<a href=\"" . $SCRIPT_NAME . "?mode=list&page=" . $params["page"] 
              . "&mapcount=" . $params["mapcount"] 
              . "&sort=" . $params["sort"]
              . "&order=" . $params["order"] . "\">"
              . $thisMap->map . "</a>");
	}
	
	if(@$serverInfo["map"] == $thisMap->map)
	{
		print("<br><a href=\"" . $prefs["REALTIME_STATUS_URL"] . "\">" 
              . $currently_in_play . "(" . $serverInfo["current"] . "/"
              . $serverInfo["max"] . ")</a>");
	}
}

/*-------------------------------------------------------------------------*/
function printThisMap_Image($prompt = "No image available")
// Prints HTML to include the map's image. If an image is not available,
//	 the function will check the value of the passed string. if it points to
//	 a vaild image (an image to display when no specific image is available)
//	 it will link to that. Otherwise the function assumes the passed value 
//	 is a text string to be displayed in place of an image. The function also
//	 adapts the hyperlink created depending on if the current view is a list
//	 mode or a single map mode. If no string is passed for $prompt, the 
//	 default value is used.
{
	global $params;
	global $thisMap;
	global $SCRIPT_NAME;

	if($params["mode"] == "list") // then link to a single map page
	{
		print("<a href=\"" . $SCRIPT_NAME . "?mode=single&map=" 
              . $thisMap->map . "\">");
	}
	else // then link to a list view
	{
		print("<a href=\"" . $SCRIPT_NAME . "?mode=list&page=" . $params["page"] 
              . "&mapcount=" . $params["mapcount"]
              . "&sort=" . $params["sort"]
              . "&order=" . $params["order"] . "\">");
	}
	
	if($thisMap->imageurl == "#na#") //then no image is available
	{
		if(@is_file(realpath($prompt))) // then $prompt is a path to an image, so display it
		{
			print("<img width=212 height=160 border=0 src=\"" . $prompt . "\"></a>");
		}
		else // $prompt is a text string to display
		{
			print($prompt . "</a>");
		}
	}
	else // then there is a vaild image available, so use it
	{
		print("<img width=212 height=160 border=0 src=\"" . $thisMap->imageurl 
              . "\" alt=\"" . $thisMap->map . "\"></a>");
	}
}

/*-------------------------------------------------------------------------*/
function printThisMap_Textfile($prompt = "No description available")
// Prints the map's description textfile. If no textfile is available for
//	 output, the $prompt is displayed in its place. If no string is passed
//	 to the function for $prompt, the default value is used.
{
	global $thisMap;
	if($thisMap->textfile == "#na#") // then no text file is available
	{
		print($prompt);
	}
	else // so try to print the text file
	{
		if($myText = @file($thisMap->textfile)) // the file was opened successfully
		{
			while(list($k, $v) = each($myText)){ print("<br>" . $v); }
		}
		else
		{
			print($prompt); // couldn't find the file
		}
	}
}

/*-------------------------------------------------------------------------*/
function printThisMap_Popularity($prompt_prefix= "Played ", 
                                 $prompt_suffix = " times",
                                 $prompt_neverPlayed = "Never played")
// Prints how many times the map has been played. Uses the prefix and suffix
//	 passed values to "cap" the output. If the map has never been played, the 
//	 $prompt_neverPlayed value is displayed. All or none of the variables must 
//	 be supplied. If none are passed, the default values are used.
{
	global $thisMap;
	if($thisMap->popularity == 0)
	{
		print($prompt_neverPlayed);
	}
	else
	{
		print($prompt_prefix . $thisMap->popularity . $prompt_suffix);
	}
}

/*-------------------------------------------------------------------------*/
function printThisMap_Mapcycle($prompt_notInCycle = "Not in mapcycle", 
                               $prompt_singleView_suffix = " in map rotation")
// Prints the map's position in the mapcycle file. If the map is not in
//	 the active rotation, the $prompt_noInCycle value is used. If the page
//	 page to be rendered will be a single map view, $prompt_singleView_suffix
//	 will be printed after the rotation number. Either both or neither of
//	 the variable must be passed! If no values are passed, the default 
//	 values are used.
{
	global $params;
	global $thisMap;
	
	if($thisMap->mapcycle == 9999)
	{
		print($prompt_notInCycle);
	}
	else
	{
		print($thisMap->mapcycle);
		if($params["mode"] == "single"){ print($prompt_singleView_suffix); }
	}
}

/*-------------------------------------------------------------------------*/
function printThisMap_Size()
// Prints the size (in kilobytes) of the map file.
{
	global $thisMap;
	print((int)($thisMap->size / 1024) . "KB");  
}

/*-------------------------------------------------------------------------*/
function printThisMap_ModDate($prompt = "Last modified on ")
// Prints the last modifed date of the map file with the passed string as
//	 a prefix to the date. If no string is passed, the default value is used.
{
	global $thisMap;
	print($prompt . date("M d, Y",$thisMap->moddate));	
}

/*-------------------------------------------------------------------------*/
function printThisMap_Download($prompt_download = "Download map", 
                               $prompt_notAvailable = "Not available")
// Prints a url to the downloadable file for this map, if available, using
//	 the $prompt_download as the text for the hyperlink. If a download is 
//	 not available, the function prints the $prompt_notAvailable variable
//	 instead. Both or neither of the two variable must be supplied. If 
//	 neither are passed to the function, the default values are used.
{
	global $thisMap;
	if($thisMap->downloadurl == "#na#")
	{
		print($prompt_notAvailable);
	}
	else
	{
		print("<a href=\"" . $thisMap->downloadurl . "\">" . $prompt_download . "</a>");
	}
}

/*#######################################################################*/?>
