/* $Id: bt-cmd.c,v 1.11 2004-07-26 17:03:46 waffel Exp $
 * You can try to run the uninstalled program via
 *   libtool --mode=execute bt-cmd <filename>
 * to enable debug output add:
 *  --gst-debug="*:2,bt-*:3" for not-so-much-logdata or
 *  --gst-debug="*:2,bt-*:4" for a-lot-logdata
 *
 * example songs can be found in ./test/songs/
 */

#define BT_CMD_C

#include "bt-cmd.h"

enum {
	ARG_SONG_FILE
};

static void init_popt_callback (poptContext context,
    enum poptCallbackReason reason,
    const GstPoptOption * option, const char *arg, void *data);

int main(int argc, char **argv) {
	gboolean res;
	BtCmdApplication *app;
  
	static struct poptOption poptTable[] = {
		 {NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_PRE | POPT_CBFLAG_POST,
        (void *) &init_popt_callback, 0, NULL, NULL},
		{"bt-song-file", '\0', POPT_ARG_STRING | POPT_ARGFLAG_STRIP, NULL,
			ARG_SONG_FILE, "Buzztard song file", "SONGFILE" },
		POPT_TABLEEND
	};
	
	// init buzztard core with own popt options
	bt_init(&argc,&argv,poptTable);

	GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-cmd", 0, "music production environment / command ui");
	
	app=(BtCmdApplication *)g_object_new(BT_TYPE_CMD_APPLICATION,NULL);
  // @todo depending on the popt options call the correct method
	res=bt_cmd_application_run(app,argc,argv);
	
	/* free application */
	g_object_unref(G_OBJECT(app));
	
	return(!res);
}

static void
init_popt_callback (poptContext context, enum poptCallbackReason reason,
    const GstPoptOption * option, const char *arg, void *data)
{
	switch (reason) {
		case POPT_CALLBACK_REASON_OPTION:
			switch (option->val) {
				case ARG_SONG_FILE:
				{
					g_printf("the path option is called with %s\n",arg);
					break;
				}
				default: {
					g_printf("unknown option\n");
				}
			}
			break;
	}
}
