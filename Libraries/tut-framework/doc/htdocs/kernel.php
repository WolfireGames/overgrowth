<?php

// say OK to user agents >>
header ($SERVER_PROTOCOL . ' 200 OK');


// validate requested URI >>
$uri = $REQUEST_URI;

if ($uri == $PHP_SELF)
	{
		header ('Location: /');
	};



// set global variables >>
// $base_dir = $DOCUMENT_ROOT . '/';
$base_dir = '/home/groups/t/tu/tut-framework/htdocs/';
$base_url = '/';

$img_dir = $base_dir . '_img/';
$img_url = $base_url . '_img/';

$src_dir = $base_dir . '_src/';
$src_url = $base_url . '_src/';

$var_dir = $base_dir . '_var/';
$var_url = $base_url . '_var/';


// set absolute paths >>
$etc = $base_dir . '_etc/';
$lib = $base_dir . '_lib/';
$xml = $base_dir . '_xml/';


// load all functions >>
$lib_array = read_dir ($lib);

for ($i = 0; $i < sizeof ($lib_array); $i++)
	{
		include ($lib . $lib_array [$i]);
	};


// load configuration >>
include ($etc . 'config.php');


// parse requested URI >>
$request_length = strlen ($uri) - strlen ($base_url);

$uri = substr ($uri, 0 - $request_length, $request_length);
$request = explode ('/', $uri);

$chapter = strtolower ($request [0]);
$query = $request [1];

if ($chapter == '')
	{
		// we are at home >>
		$target = $xml . 'index.xml';
		$home = $project_name;
	}
else
	{
		if ($query == '')
			{
				// we are NOT at home >>
				$target = $xml . $chapter . '.xml';
				$home = '<a href="/">' . $project_name . '</a>';
			}
		else
			{
				if ($chapter == 'get')
					{
						// someone wants to get file >>
						$get = get ($query);
						header ('Location: ' . $get);
						exit;
					}
				else
					{
						// error - don't know what to do with query >>
						$error = TRUE;
					};
			};
	};


// get main menu >>
$_ = file ($xml . 'content.txt');

for ($i = 0; $i < sizeof ($_); $i++)
	{
		$_ [$i] = trim ($_ [$i]);
		
		if ($_ [$i] != '')
			{
				list ($menu_section, $menu_file) = explode ('<--!-->', $_ [$i]);
				list ($menu_file_name, $menu_file_extention) = explode ('.', $menu_file);
				
				if (is_file ($xml . $menu_file))
					{
						$chapter_in_file [$xml . $menu_file] = TRUE;
						$chapter_in_menu [$xml . $menu_file] = TRUE;
						
						$menu_structure [$menu_section] [] = $menu_file_name;
					};
			};
	};


# format main menu >>
$sections = array_keys ($menu_structure);

for ($i = 0; $i < sizeof ($sections); $i++)
	{
		$menu .= '<p class="subheader">' . $sections [$i] . "</p>\n<ul>\n";
		
		$_ = $menu_structure [$sections [$i]];
		
		for ($j = 0; $j < sizeof ($_); $j++)
			{
				if ($_ [$j] != '')
					{
						$parsed_chapter = parse_xml ($xml . $_ [$j] . '.xml');
						
						if ($_ [$j] == $chapter)
							{
								// current chapter >>
								$menu .= '<li><p><strong>' . $parsed_chapter ['chapter'] . '</strong><br>' . $parsed_chapter ['annotation'] . "</p></li>\n";
								// define description meta tag for current page >>
								$description = $parsed_chapter ['chapter'] . ' - ' . $parsed_chapter ['annotation'];
							}
						else
							{
								// NOT current chapter >>
								$menu .= '<li><p><a href="/' . $_ [$j] . '/" class="menu">' . $parsed_chapter ['chapter'] . '</a><br>' . $parsed_chapter ['annotation'] . "</p></li>\n";
							};
					};
			};
		
		$menu .= "</ul>\n";
	};

// make content >>
$_ = parse_xml ($target);


$title = $project_name;
$header = $_ ['chapter'];
$content = $_ ['content'];


if ($chapter != '')
	{
		if ($chapter_in_file [$target] === TRUE && $chapter_in_menu [$target] === TRUE)
			{
				// chapter exist and in menu >>
				$title = $header . ' - ' . $title;
			}
		else
			{
				// chapter NOT exist or NOT in menu >>
				$error = TRUE;
			};
	};


// handle error >>
if ($error === TRUE)
	{
		$header = 'Error 404: file not found';
		$content = '<p>Error 404: file not found</p>';
		$title = $header . ' - ' . $title;
	};


// get downloadable files >>
$ls_arg = trim (get_tag ($content, 'ls'));

if ($ls_arg != '')
	{
		$ls = ls ($ls_arg);
		$_ = explode ('<ls>' . $ls_arg . '</ls>', $content);
		$content = implode ($ls, $_);
	};




// make style >>
$style = read_file ($etc . 'master.css');


// load page design >>
include ($etc . 'skin.php');




// KERNEL FUNCTIONS >>
// ----------------------------------------------------- //


// read directory handle and return array from file names >>
function read_dir ($dir)
	{
		if (is_dir ($dir))
			{
				if (substr ($dir, -1) != '/')
					{
						$dir = $dir . '/';
					};
				
				$handle = opendir ($dir);
				
				while (($file_name = readdir ($handle)) !== FALSE)
					{
						if (!is_dir ($dir . $file_name) && substr ($file_name, 0, 1) != '.')
							{
								$out [] = $file_name;
							};
					};
				
				closedir ($handle);
				
				rsort ($out);
				reset ($out);
				
				return $out;
			}
		else
			{
				return FALSE;
			};
	};


// read file and return STRING with file content >>
function read_file ($file_name)
	{
		if (is_file ($file_name))
			{
				$_ = file ($file_name);
				$content = implode ('', $_);
				
				return $content;
			}
		else
			{
				return FALSE;
			};
	};


// ----------------------------------------------------- //
// KERNEL FUNCTIONS <<

?>
