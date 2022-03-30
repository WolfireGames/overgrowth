<?php

# define used xml tags >>
$xml_tags = array
	(
		'section',
		'chapter',
		'annotation',
		'content',
	);

# parse string and return value of given tag >>
function get_tag ($in, $tag)
	{
		$_ = explode ('</' . $tag . '>', $in);
		$__ = explode ('<' . $tag . '>', $_ [0]);
		
		
		$out = trim ($__ [1]);
		
		
		return $out;
	};


# parse file and return attay - 'tag_title' => 'tag_value' >>
function parse_xml ($file)
	{
		global $xml_tags;
		
		
		$in = read_file ($file);
		$in = trim ($in);
		
		
		for ($i = 0; $i < sizeof ($xml_tags); $i++)
			{
				$out [$xml_tags [$i]] = get_tag ($in, $xml_tags [$i]);
			};
		
		
		return $out;
	};


?>