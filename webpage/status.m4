m4_divert(-1)
m4_define(`__TITLE',`Status')
m4_define(`__NODE',m4___file__)
m4_define(`__META_DESCRIPTION',`current status of the buzztard project (replacement of the buzz music composer)')
m4_define(`__META_KEYWORDS',`current status, project state')
m4_define(`__DATE_CREATED',`Thu Dec 18 17:07:20 MET 2003')
m4_define(`__DATE_CHANGED',`Tue Apr 27 20:03:58 CEST 2004')
m4_define(`__HEADER_EXTRA',`')
m4_include(`global.m4')
m4_divert`'m4_dnl
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="en">
	__HEADER
  <body bgcolor="#FFFFFF">
		__NAVBAR
		<div id="main">
			<h3>Project State</h3>
			<p align="justify">
				The project has just started. We are currently discussing the design and
				develop the first test programs. The current CVS already holds the whole
				auto* structure and some code.<br>
				Buzztard will use glibs GObject framework (
				<a href="http://www.le-hacker.org/papers/gobject/index.html" target="_new">a good introduction to GObject</a>
				) and <a href="http://gstreamer.freedesktop.org/" target="_new">gstreamer</a>
				will be the base media framework we will use.				
			</p>
			<h3>Tasks</h3>
			<p align="justify">
				Below we list a few initial tasks and how far we progressed with them. Your
				help is very welcome (especially with the tasks not yet started). So if
				you are interested, please contact <a href="mailto&#58;ensonic&#64;users&#46;sourceforge&#46;net">us</a>
				<ol>
					<li><b>song network via gstreamer</b> (ensonic)<br>
						I can create simple pipelines/threads. The examples can
						dynamically change parameters (pitch, volume).<br>
						Currently I work on a high level connection framework
						(automatically handling 1-n, n-1 connections and format differences).
						This can already plat simple pattern based sequences.
					</li>
					<li><b>querying plugins from gstreamer installation</b> (waffel)<br>
						The task is finding out what plugins are available by media type
						(e.g. only audio)	and by plugin type (e.g. sources, sinks,
						converter/effects).<br>
						I can currently festch the data, but not sure if that is the proper way (looks like a private API).
					</li>
					<li><b>writting a gstreamer plugin that wraps buzz machines</b> ()<br>
						This should work like the ladspa wrapper, but needs to be able to wrap
						windows dlls (the dll api is knwon to us).<br>
						A starting point is to look at libtool or apps like mplayer, wine.
						A coding expample that needs to be evaluated can be found
						<a href="http://home.sch.bme.hu/~marosi/dllnt.source/" target="_new">at this site</a>.
					</li>
					<li><b>writting gstreamer mem-sink, mem-source plugins</b> ()<br>
						We need this plugin to load and save samples in any gst supported format.
					</li>
					<li><b>using zlib to read and write zip-archives contains song-files and binary data</b> ()<br>
						The plan is to store the song-data in xml format, but to include the digital audio data.
						That could be done storing a song as a zip archive, which contains all required files.<br>
						<a href="http://www.info-zip.org/pub/infozip/" target="_new">Zlib</a> already has
						some example code (see contrib/minizip).
					</li>
				</ol>
			</p>
			<h3>Join the discussion</h3>
			<p align="justify">
				If you have any comments, ideas and suggestions, besides mailing
				<a href="mailto&#58;ensonic&#64;users&#46;sourceforge&#46;net">me</a>, you can join the
				<a href="http://sourceforge.net/mail/?group_id=55124" target="_blank">project mailing lists</a>.
			</p>
		</div>
		__FOOTER
  </body>
</html>
