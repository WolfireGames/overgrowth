<?php


function get ($arg = '')
	{
		global $var_dir;
		global $var_url;
		
		
		$counter = 'counter.txt';
		$div = "\011";
		
		
		if (strtolower ($arg) == 'counter')
			{
				// show statisctics >>
				$out = $var_url . $counter;
			}
		else
			{
				// update counter and get file >>
				global $src_dir;
				global $src_url;
			
				$out = $src_url . $arg;
			};
		
		
		return $out;
	};




function ls ($arg = 'all')
	{
		global $src_dir;
		global $src_url;
		
		
		if ($arg != 'all')
			{
				if (eregi ("[1-9]+[0-9]*", $arg))
					{
						$number_of_releases = $arg;
					}
				elseif (substr ($arg, 0, 1) == '.')
					{
						$allowed_extention = $arg;
					};
			};
		
		
		$src_array = read_dir ($src_dir);
		
		
		for ($i = 0; $i < sizeof ($src_array); $i++)
			{
				$_ = explode ('.', $src_array [$i]);
				
				$src_unique [$_ [0]] .= $src_array [$i] . "\n";
			};
		
		
		$release = array_keys ($src_unique);
		
		
		if (!isset ($number_of_releases))
			{
				$number_of_releases = sizeof ($release);
			};
		
		
		for ($i = 0; $i < $number_of_releases; $i++)
			{
				$_ = trim ($src_unique [$release [$i]]);
				
				
				$file = explode ("\n", $_);
				
				
				for ($j = 0; $j < sizeof ($file); $j++)
					{
						$offset = strlen ($release [$i]) - strlen ($file [$j]);
						$ext = substr ($file [$j], $offset);
						
						
						if ($ext == '.xml')
							{
								// release changes >>
								$changes = trim (get_tag (read_file ($src_dir . $file [$j]), 'changes'));
							}
						else
							{
								// downloadable files >>
								if (isset ($allowed_extention))
									{
										if ($ext == $allowed_extention)
											{
												$link [] = '<a href="/get/' . $file [$j] . '">' . $file [$j] . '</a>';
											};
									}
								else
									{
										$link [] = '<a href="/get/' . $file [$j] . '">' . $file [$j] . '</a>';
									};
							};
					};
				
				
				if (sizeof ($link) > 0)
					{
						sort ($link);
						reset ($link);
						
						
						if ($changes != '')
							{
								$changes = "<p>Changes:</p>\n" . $changes . "\n";
							};
						
						
						$ls [] = "<p><strong>" . $release [$i] . "</strong></p>\n" . $changes . "<p>Download:</p>\n<ul>\n<li>" . implode ("<br></li>\n<li>", $link) . "<br></li>\n</ul>\n";
					};
				
				
				unset ($changes);
				unset ($link);
			};
		
		
		if (sizeof ($ls) > 0)
			{
				$out = "<!-- downloadable files >> -->\n" . implode ('', $ls) . "<!-- downloadable files << -->\n";
			};
		
		
		return $out;
	};


?>