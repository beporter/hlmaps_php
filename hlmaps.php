<?php /*###################################################################*/
/*# hlmaps.php                                                            #*/
/*#  - hlmaps main routine                                                #*/
/*#  - Copyright Scott McCrory and Brian Porter                           #*/
/*#  - Distributed under the GPL terms - see docs for info                #*/
/*#  - http://hlmaps.sourceforge.net                                      #*/
/*#                                                                       #*/
/*#########################################################################*/
//#
//#    HLmaps-php
//#    Copyright (C) 2002 Scott McCrory and Brian Porter
//#    All Rights Reserved
//#
//#    This program is free software; you can redistribute it and/or modify
//#    it under the terms of the GNU General Public License as published by
//#    the Free Software Foundation; either version 2 of the License, or
//#    (at your option) any later version.
//#
//#    This program is distributed in the hope that it will be useful,
//#    but WITHOUT ANY WARRANTY; without even the implied warranty of
//#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//#    GNU General Public License for more details.
//#
//#    You should have received a copy of the GNU General Public License
//#    along with this program; if not, write to the Free Software
//#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  
//#    02111-1307  USA
//#
//###########################################################################

/***************************************************************************/
/* CHANGE THIS TO POINT TO YOUR "hlmaps.inc"                               */
// Absolute filesystem path to include file
//   i.e.: "/etc/hlmaps/hlmaps.inc" or "C:/hlmaps/hlmaps.inc"
$INCLUDE_FILE = "C:/hlmaps-php/php/hlmaps.inc";



/***************************************************************************/
/****                                                                   ****/
/****             # NO NEED TO EDIT BELOW THIS POINT #                  ****/
/****                                                                   ****/
/***************************************************************************/

/***************************************************************************/
/* Global Development Constants                                            */
/***************************************************************************/
// Development constants - please don't mess with these
$VERSION             = "2.0b1, February 20, 2002";
$AUTHOR_NAME         = "Brian Porter";
$AUTHOR_EMAIL        = "beporter@users.sourceforge.net";
$HOME_PAGE           = "http://hlmaps.sourceforge.net";
//$SCRIPT_NAME         = $SCRIPT_NAME; // Auto-defined by PHP in global scope
$PACKAGE_NAME        = "HLmaps_PHP"; // Use this for credits instead

/***************************************************************************/
/* Main                                                                    */
/***************************************************************************/

class mapClass
{
  var $map;
  var $imageurl;
  var $downloadurl;
  var $textfile;
  var $popularity;
  var $mapcycle;
  var $size;
  var $moddate;
}

class playerClass
{
  var $name;
  var $frags;
  var $time;
}

// Open (and parse) the include file (which will also parse and load the 
//   config file for us, and tell us where our template files are.)
require($INCLUDE_FILE);

// Create an empty associative array to hold the preferences and parameters passed by URL
$prefs = array();  
$params = array(); 

// Create arrays to hold server info, rules, and players from live status
$serverInfo = array();
$serverRules = array();
$playerList = array();

// Create an array to hold all the map data imported from the data source
//   and an integer to quickly keep track of the number of maps inside
$allMaps = array();
$numOfMaps = 0;

// We also need an array to hold the index values of maps from $allMaps we want
//   to display on the current page based on viewing mode and sort order
$displayedMaps = array();

// Lastly, a global map container used by the printThisMap_...() functions
$thisMap = new mapclass;  


// Set up the current environment
setDefaultValues();
getPreferences();
getParameters();
getRealtimeServerStatus();

// Load all map data from the data source
if($prefs["DATA_LOCATION"] === "mysql")
{ 
  getMapDetails_mysql();
}
else
{ 
  getMapDetails_text();
}

// Grab only the records we need for displaying the current page
populateMapsForPage();

// Determine which template file we use based on the arguments passed 
//   in the URL and insert our template file here-- it does the rest 
//   of the work for us.
switch($params["mode"])
{
  case "single":  // only one map to display
       include($prefs["SINGLE_TEMPLATE_FILE"]);
       break;
  case "list":    // show a list of maps
  default:        // no parameters in URL, so default to a list view
       include($prefs["LIST_TEMPLATE_FILE"]);
       break;
}
printCredits();  // If we haven't already, print credit information.
/*#######################################################################*/?>
