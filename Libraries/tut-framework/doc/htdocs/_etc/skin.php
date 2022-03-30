<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">

<html>
<head>
<title><?php print $title; ?></title>
<meta name="keywords" content="<?php print $keywords; ?>">
<meta name="description" content="<?php print $description; ?>">
<?php print $style; ?>
</head>

<body text="#000000" link="#ff9900" alink="#ffcc00" vlink="#ff9900" bgcolor="#ffffff" leftmargin=0 topmargin=0 marginheight=0 marginwidth=0>

<img src="/_img/pixel.gif" alt="" width=1 height=20><br>
<table width="100%" cellspacing=0 cellpadding=0>
<tr>
<td width=20 align=left valign=top>
<img src="/_img/pixel.gif" alt="" width=20 height=1><br>
</td>
<td width="100%" align=left valign=top>
<!-- home table >> -->
<table width="100%" cellspacing=0 cellpadding=0>
<tr>
<td width="100%" align=center valign=center bgcolor="#cccccc">
<!-- >> -->
<table width="100%" cellspacing=1 cellpadding=10>
<tr>
<td width="100%" align=left valign=top bgcolor="#eeeeee">
<p class="logo"><?php print $home; ?></p>
</td>
</tr>
</table>
<!-- << -->
</td>
</tr>
</table>
<!-- home table << -->
</td>
<td width=20 align=left valign=top>
<img src="/_img/pixel.gif" alt="" width=20 height=1><br>
</td>
</tr>
</table>

<table border=0 width="100%" cellspacing=20 cellpadding=0>
<tr>
<td width="30%" align=left valign=top>
<!-- menu table >> -->
<table width="100%" cellspacing=0 cellpadding=0>
<tr>
<td width="100%" align=center valign=center bgcolor="#cccccc">
<!-- >> -->
<table width="100%" cellspacing=1 cellpadding=10>
<tr>
<td width="100%" align=left valign=top bgcolor="#eeeeee">
<?php print $menu; ?>
</td>
</tr>
</table>
<!-- << -->
</td>
</tr>
</table>
<!-- menu table << -->
</td>
<td width="70%" valign=top>
<!-- content table >> -->
<table width="100%" cellspacing=0 cellpadding=0>
<tr>
<td width="100%" align=center valign=center bgcolor="#cccccc">
<!-- >> -->
<table width="100%" cellspacing=1 cellpadding=10>
<tr>
<td width="100%" align=left valign=top bgcolor="#eeeeee">
<p class="header"><?php print $header; ?></p>
<?php print $content; ?>

<img src="/_img/pixel.gif" alt="" width=1 height=300><br>

<center>
<!--RAX counter-->
<script language="JavaScript">
<!--
document.write('<a href="http://www.rax.ru/click" '+
'target=_blank><img src="http://counter.yadro.ru/hit?t13.4;r'+
escape(document.referrer)+((typeof(screen)=='undefined')?'':
';s'+screen.width+'*'+screen.height+'*'+(screen.colorDepth?
screen.colorDepth:screen.pixelDepth))+';'+Math.random()+
'" alt="rax.ru: показано число хитов за 24 часа, посетителей за 24 часа и за сегодн\я" '+
'border=0 width=88 height=31></a><br>')
//-->
</script>
<!--/RAX-->
</center>

</td>
</tr>
</table>
<!-- << -->
</td>
</tr>
</table>
<!-- content table << -->
</td>
</tr>
</table>

</body>
</html>
