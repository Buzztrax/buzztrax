m4_dnl !!! IMPORTANT !!!
m4_dnl Do not reformat paragraphs !
m4_dnl m4 does onely *one* macro expansion per line !
m4_dnl !!! IMPORTANT !!!


m4_define(`__PACKAGE',`Buzztard - Music Production Environment')


m4_dnl -- header definition ----------------------------------------------------

m4_define(`__HEADER',`
	<head>
		<title>__PACKAGE __TITLE</title>
		<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
		__HEADER_EXTRA
		<link href="screen.css" rel="stylesheet" type="text/css">
 		<link rel="shortcut icon" href="favicon.ico">
		<meta name="description" content="gitk project homepage : __META_DESCRIPTION">
		<meta name="keywords" content="buzztard, buzz, realtime music composer, __META_KEYWORDS">
		<meta name="author" content="Stefan Kost">
	</head>
')m4_dnl


m4_dnl -- site navigation ------------------------------------------------------

m4_define(`__NAVBUTTON_ARCHITECTURE',`
	m4_ifelse(m4_index(__NODE,`architecture'),0,
	`<a id="menua" href="architecture.html">Architecture</a>',
	`<a id="menu" href="architecture.html">Architecture</a>')
')m4_dnl

m4_define(`__NAVBUTTON_DEVELOPMENT',`
	`<a id="menu" href="/phpwiki/index.php/Welcome%20Developers%21">Development</a>'
')m4_dnl

m4_define(`__NAVBUTTON_LINKS',`
	`<a id="menu" href="/phpwiki/index.php/Related%20Software%20Packages">Links</a>'
')m4_dnl

m4_define(`__NAVBUTTON_OVERVIEW',`
	`<a id="menu" href="/phpwiki/index.php/Overview">Overview</a>'
')m4_dnl

m4_define(`__NAVBUTTON_STATUS',`
	`<a id="menu" href="/phpwiki/index.php/Overview">Status</a>'
')m4_dnl

m4_dnl when changing the menu do
m4_dnl ssh ensonic@shell.sourceforge.net
m4_dnl vi ./themes/buzztard/templates/sitehead.tmpl

m4_define(`__NAVBAR',`
		<div id="nav">
			<table border="0" width="100%" cellspacing="0" cellpadding="0">
				<tr>
					<td valign="bottom">
						<img align="left" src="/buzztard.gif">
						<div style="position:absolute; left:122px; top:17px; z-index:1;">
							<h1 style="color:#aaaaaa;">Buzztard - <br>Music Production Environment</h1>
						</div>
						<div style="position:absolute; left:121px; top:16px; z-index:2;"> 
							<h1 style="color:#555555;">Buzztard - <br>Music Production Environment</h1>
						</div>
						<div style="position:absolute; left:120px; top:15px; z-index:3;"> 
							<h1>Buzztard - <br>Music Production Environment</h1>
						</div>
					</td>
					<td valign="top" align="right">
						<div id="navi">
							::: Menu<br><br>
							__NAVBUTTON_OVERVIEW<br>
							__NAVBUTTON_STATUS<br>
							__NAVBUTTON_DEVELOPMENT<br>
							__NAVBUTTON_LINKS<br>
						</div>
					</td>
				</tr>
			</table>
		</div>
')m4_dnl


m4_dnl -- footer definition ----------------------------------------------------

m4_define(`__FOOTER',`
    <div id="foot">
			<table width="100%" cellspacing="0" cellpadding="0">
				<tr>
					<td align="left">
						<font size="-1">
							Send mail to: <a href="mailto&#58;ensonic&#64;users&#46;sourceforge&#46;net">Stefan Kost</a><br>
							<!-- Created: 
							__DATE_CREATED
							-->Last modified: 
							__DATE_CHANGED
						</font>
					</td>
					<td align="right">
						<a href="http://validator.w3.org/check/referer"><img border="0"	src="http://www.w3.org/Icons/valid-html401" alt="Valid HTML 4.01!" height="31" width="88"></a>
						<a href="http://jigsaw.w3.org/css-validator/"><img style="border:0;width:88px;height:31px" src="http://jigsaw.w3.org/css-validator/images/vcss" alt="Valid CSS!"></a>
						<a href="http://sourceforge.net"><img src="http://sourceforge.net/sflogo.php?group_id=55124" width="88" height="31" border="0" alt="SourceForge Logo"></a>
					</td>
				</tr>
			</table>
		</div>
')m4_dnl

m4_dnl -- eof ------------------------------------------------------------------

