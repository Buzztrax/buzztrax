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
	m4_ifelse(m4_index(__NODE,`development'),0,
	`<a id="menua" href="development.html">Development</a>',
	`<a id="menu" href="development.html">Development</a>')
')m4_dnl

m4_define(`__NAVBUTTON_LINKS',`
	m4_ifelse(m4_index(__NODE,`links'),0,
	`<a id="menua" href="links.html">Links</a>',
	`<a id="menu" href="links.html">Links</a>')
')m4_dnl

m4_define(`__NAVBUTTON_OVERVIEW',`
	m4_ifelse(m4_index(__NODE,`overview'),0,
	`<a id="menua" href="overview.html">Overview</a>',
	`<a id="menu" href="overview.html">Overview</a>')
')m4_dnl

m4_define(`__NAVBUTTON_PUBLICATIONS',`
	m4_ifelse(m4_index(__NODE,`publications'),0,
	`<a id="menua" href="publications.html">Publications</a>',
	`<a id="menu" href="publications.html">Publications</a>')
')m4_dnl

m4_define(`__NAVBUTTON_STATUS',`
	m4_ifelse(m4_index(__NODE,`status'),0,
	`<a id="menua" href="status.html">Status</a>',
	`<a id="menu" href="status.html">Status</a>')
')m4_dnl

m4_define(`__NAVBUTTON_WIKI',`
	<a id="menu" href="/phpwiki/">Wiki</a>
')m4_dnl

m4_define(`__NAVBAR',`
		<div id="nav">
			<table border="0" width="100%" cellspacing="0" cellpadding="0">
				<tr>
					<td valign="bottom">
						<!--img align="left" src="buzztard.gif"-->
						<h1>Buzztard - <br>Music Production Environment</h1>
					</td>
					<td valign="top" align="right">
						<div id="navi">
							::: Menu<br><br>
							__NAVBUTTON_OVERVIEW<br>
							<!--__NAVBUTTON_ARCHITECTURE<br>-->
							__NAVBUTTON_STATUS<br>
							__NAVBUTTON_DEVELOPMENT<br>
							<!--__NAVBUTTON_LINKS><br-->
							__NAVBUTTON_WIKI<br>
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
						<a href="http://sourceforge.net"><img src="http://sourceforge.net/sflogo.php?group_id=55124" width="88" height="31" border="0" alt="SourceForge Logo"></a>
					</td>
				</tr>
			</table>
		</div>
')m4_dnl

m4_dnl -- eof ------------------------------------------------------------------

