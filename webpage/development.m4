m4_divert(-1)
m4_define(`__TITLE',`Development')
m4_define(`__NODE',m4___file__)
m4_define(`__META_DESCRIPTION',`development of the buzztard project (replacement of the buzz music composer)')
m4_define(`__META_KEYWORDS',`cvs access, builing, install, api documentation')
m4_define(`__DATE_CREATED',`Thu Jun 21 17:31:23 MET 2001')
m4_define(`__DATE_CHANGED',`Fri Dec 05 07:58:49 MET 2003')
m4_define(`__HEADER_EXTRA',`')
m4_include(`global.m4')
m4_divert`'m4_dnl
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="en">
	__HEADER
  <body bgcolor="#FFFFFF">
		__NAVBAR
		<div id="main">
			<h3>Sourceforge Page</h3>
			<p align="justify">
				Development takes place at <a href="http://sourceforge.net/projects/buzztard/">http://sourceforge.net/projects/buzztard/</a>.
			</p>
	
			<h3><acronym title="Concurrent Versioning System">CVS</acronym></h3>
			<!--p align="justify"-->
				To login, do:<br>
				<div id="src">
					&nbsp;&nbsp;cvs -d:pserver:anonymous@cvs.buzztard.sourceforge.net:/cvsroot/buzztard login<br>
				</div>
				To check out the source, do:<br>
				<div id="src">
					&nbsp;&nbsp;cvs -z3 -d:pserver:anonymous@cvs.buzztard.sourceforge.net:/cvsroot/buzztard co buzztard<br>
				</div>
				To updated the source, do (from within each directory):<br>
				<div id="src">
					&nbsp;&nbsp;cvs -z3 update -dP
				</div>
			<!--/p-->
		</div>
		__FOOTER
  </body>
</html>
