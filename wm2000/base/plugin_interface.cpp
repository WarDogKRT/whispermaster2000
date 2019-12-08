/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) 2008-2017 TeamSpeak Systems GmbH
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "base/plugin_base.h"
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "base/plugin.h"
#include "misc/error_handler.h"

char pPluginPath[PATH_BUFSIZE];

static struct TS3Functions ts3Functions;
static char* pluginID = NULL;

static plugin_base          cPluginBase;
error_handler               cErrHandler;      // link to error handler

#define DEBUG_TSIF 1

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif


/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name()
{
	return  cPluginBase.get_name();
}

/* Plugin version */
const char* ts3plugin_version()
{
	return cPluginBase.get_version();
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion()
{
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author()
{
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return cPluginBase.get_user();
}


/* Plugin description */
const char* ts3plugin_description()
{
    return cPluginBase.get_description();
}


/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs)
{
    ts3Functions = funcs;
    return;
}


/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init()
{
    char appPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
	

    /* Your plugin init code here */
    if(DEBUG_TSIF) printf("PLUGIN: init\n");

	

    /* Example on how to query application, resources and configuration paths from client */
    /* Note: Console client returns empty string for app and resources path */
    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pPluginPath, PATH_BUFSIZE, pluginID);

    if (DEBUG_TSIF) printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pPluginPath);

    //init error handler
    cErrHandler.init(std::string(pPluginPath), ts3Functions.logMessage);
    cErrHandler.remove_log_file();

    // read config from file
    cPluginBase.Init(ts3Functions, std::string(pPluginPath), pluginID);

    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown()
{
    /* Your plugin cleanup code here */
    try
    {
        if (DEBUG_TSIF) printf("PLUGIN: start shutdown\n");

	    //save last config to file
        cPluginBase.Close();
        if (DEBUG_TSIF) printf("PLUGIN: shutdown...\n");
        

	    /* Free pluginID if we registered it */
	    if(pluginID != NULL)
        {
		    free(pluginID);
		    pluginID = NULL;
	    }
        if (DEBUG_TSIF) printf("PLUGIN: shutdown done\n");
    }
    catch (std::exception &e)
    {
        cErrHandler.error_log(__FUNCSIG__, e);
    }
    catch (boost::exception &e)
    {
        cErrHandler.error_log(__FUNCSIG__, e);
    }
    catch (...)
    {
        cErrHandler.error_log(__FUNCSIG__);
    }
    return;
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure()
{
    if (DEBUG_TSIF) printf("PLUGIN: offersConfigure\n");
	/*
	 * Return values:
	 * PLUGIN_OFFERS_NO_CONFIGURE         - Plugin does not implement ts3plugin_configure
	 * PLUGIN_OFFERS_CONFIGURE_NEW_THREAD - Plugin does implement ts3plugin_configure and requests to run this function in an own thread
	 * PLUGIN_OFFERS_CONFIGURE_QT_THREAD  - Plugin does implement ts3plugin_configure and requests to run this function in the Qt GUI thread
	 */
	return PLUGIN_OFFERS_CONFIGURE_QT_THREAD;  /* In this case ts3plugin_configure does not need to be implemented */
}

/* Plugin might offer a configuration window. If ts3plugin_offersConfigure returns 0, this function does not need to be implemented. */
void ts3plugin_configure(void* handle, void* qParentWidget)
{
    if (DEBUG_TSIF) printf("PLUGIN: configure handle==0x%llX / qParentWidget==0x%llX\n", (uint64)handle, (uint64)qParentWidget);
    cPluginBase.open_configure_ui();
    return;
}

/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id)
{
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
    if (DEBUG_TSIF) printf("PLUGIN: registerPluginID: %s\n", pluginID);
    return;
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword()
{
	return "WhisperMaster";
}


/* Plugin processes console command. Return 0 if plugin handled the command, 1 if not handled. */
int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command)
{
	return 0;  /* Plugin handled command */
}

/* Client changed current server connection handler */
void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID)
{
    if (DEBUG_TSIF) printf("PLUGIN: currentServerConnectionChanged %llu (%llu)\n", (long long unsigned int)serverConnectionHandlerID, (long long unsigned int)ts3Functions.getCurrentServerConnectionHandlerID());
    return;
}

/*
 * Implement the following three functions when the plugin should display a line in the server/channel/client info.
 * If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
 */

/* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle()
{
	return cPluginBase.get_infoTitle();
}

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data)
{
    cPluginBase.infoData(serverConnectionHandlerID, id, type, data);
    return;
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data)
{
    try
    {
        free(data);
    }
    catch (std::exception &e)
    {
        cErrHandler.error_log(__FUNCSIG__, e);
    }
    catch (boost::exception &e)
    {
        cErrHandler.error_log(__FUNCSIG__, e);
    }
    catch (...)
    {
        cErrHandler.error_log(__FUNCSIG__);
    }
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload()
{
	return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/*
 * Initialize plugin menus.
 * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
 * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
 * If plugin menus are not used by a plugin, do not implement this function or return NULL.
 */
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon)
{
	/*
	 * Create the menus
	 * There are three types of menu items:
	 * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
	 * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
	 * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
	 *
	 * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
	 *
	 * The menu text is required, max length is 128 characters
	 *
	 * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
	 * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
	 * plugin filename, without dll/so/dylib suffix
	 * e.g. for "whispermaster2000_x86/x64.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\WhisperMaster2000\1.png
	 */
    cPluginBase.InitMenu(menuItems);

	/*
	 * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
	 * If unused, set menuIcon to NULL
	 */
    try
    {
	    *menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	    _strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "phone.png");
    }
    catch (std::exception &e)
    {
        cErrHandler.error_log(__FUNCSIG__, e);
    }
    catch (boost::exception &e)
    {
        cErrHandler.error_log(__FUNCSIG__, e);
    }
    catch (...)
    {
        cErrHandler.error_log(__FUNCSIG__);
    }
	/*
	 * Menus can be enabled or disabled with: ts3Functions.setPluginMenuEnabled(pluginID, menuID, 0|1);
	 * Test it with plugin command: /test enablemenu <menuID> <0|1>
	 * Menus are enabled by default. Please note that shown menus will not automatically enable or disable when calling this function to
	 * ensure Qt menus are not modified by any thread other the UI thread. The enabled or disable state will change the next time a
	 * menu is displayed.
	 */
	/* For example, this would disable MENU_ID_GLOBAL_2: */
	/* ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0); */

	/* All memory allocated in this function will be automatically released by the TeamSpeak client later by calling ts3plugin_freeMemory */
}





/*
 * Initialize plugin hotkeys. If your plugin does not use this feature, this function can be omitted.
 * Hotkeys require ts3plugin_registerPluginID and ts3plugin_freeMemory to be implemented.
 * This function is automatically called by the client after ts3plugin_init.
 */
void ts3plugin_initHotkeys(struct PluginHotkey*** hotkeys)
{
	/* Register hotkeys giving a keyword and a description.
	 * The keyword will be later passed to ts3plugin_onHotkeyEvent to identify which hotkey was triggered.
	 * The description is shown in the clients hotkey dialog. */
    cPluginBase.initHotkeys(hotkeys);

	/* The client will call ts3plugin_freeMemory to release all allocated memory */
}

/************************** TeamSpeak callbacks ***************************/
/*
 * Following functions are optional, feel free to remove unused callbacks.
 * See the clientlib documentation for details on each function.
 */

/* Clientlib */
void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber)
{
    /* Some example code following to show how to use the information query functions. */
    cPluginBase.onConnect(serverConnectionHandlerID, newStatus);
    return;
}

void ts3plugin_onNewChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID)
{
	//initialisation of channel by channel => use ts3plugin_onConnectStatusChangeEvent(...) instead
    //if (DEBUG_TSIF) printf("ts3plugin_onNewChannelEvent \n");
}

void ts3plugin_onNewChannelCreatedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier)
{
	//on creation of new sub-/channel
    if (DEBUG_TSIF) printf("ts3plugin_onNewChannelCreatedEvent: channelID %llu, channelParentID %llu, invokerID %d, invokerName %s\n", channelID, channelParentID, invokerID, invokerName);
}

void ts3plugin_onDelChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier)
{
	//on (auto-)delete of a channel
    if (DEBUG_TSIF) printf("ts3plugin_onDelChannelEvent: channelID %llu, invokerID %d, invokerName %s\n", channelID, invokerID, invokerName);
}

void ts3plugin_onChannelMoveEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 newChannelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier)
{
	//if channel is moved
    if (DEBUG_TSIF) printf("ts3plugin_onChannelMoveEvent: channelID %llu, newChannelParentID %llu, invokerID %d, invokerName %s\n", channelID, newChannelParentID, invokerID, invokerName);
}

void ts3plugin_onUpdateChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID)
{
	//if user clicks on a channel
    if (DEBUG_TSIF) printf("ts3plugin_onUpdateChannelEvent \n");
}

void ts3plugin_onUpdateChannelEditedEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier)
{
    if (DEBUG_TSIF) printf("ts3plugin_onUpdateChannelEditedEvent \n");
}

void ts3plugin_onUpdateClientEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier)
{
	//if user clicks on a client
    if (DEBUG_TSIF) printf("onUpdateClientEvent(clientID %d) => ", clientID);

    //connection state is always == connected
    cPluginBase.onUpdateClientEvent(serverConnectionHandlerID, clientID, INVALID_CHANNEL_ID);
}

void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage)
{
    //on connect of other client	(oldChannelID 0, visibility 0)
    //if user selects other channel	(visibility 1)
    //on disconnect of other client	(newChannelID 0, visibility 2)
    if (DEBUG_TSIF) printf("onClientMoveEvent (clientID %d, newChannelID %llu) => ", clientID, newChannelID);

    //if newChannelID == 0 => disconnect
    cPluginBase.onUpdateClientEvent(serverConnectionHandlerID, clientID, newChannelID);
}

void ts3plugin_onClientMoveSubscriptionEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility)
{
    //called on connect of own client
    if (DEBUG_TSIF) printf("onSubscriptionEvent(clientID %d, newChannelID %llu) => ", clientID, newChannelID);

    //channel id is unknown here, set to -1 to prevent update function to overwrite the value
    cPluginBase.onUpdateClientEvent(serverConnectionHandlerID, clientID, newChannelID);
}

void ts3plugin_onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage)
{
    //if other client times out
    if (DEBUG_TSIF) printf("onClientMoveTimeoutEvent (clientID %d, newChannelID %llu) => ", clientID, newChannelID);

    //if newChannelID == 0 => disconnect
    cPluginBase.onUpdateClientEvent(serverConnectionHandlerID, clientID, newChannelID);
}

void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, const char* moveMessage)
{
	// if user is automoved after (sub-)channel creation
    // if user is manually moved by other user
    if (DEBUG_TSIF) printf("onClientMoveMovedEvent (clientID %d, newChannelID %llu) => ", clientID, newChannelID);
    cPluginBase.onUpdateClientEvent(serverConnectionHandlerID, clientID, newChannelID);
}

void ts3plugin_onClientKickFromChannelEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage)
{
    if (DEBUG_TSIF) printf("ts3plugin_onClientKickFromChannelEvent \n");
}

void ts3plugin_onClientKickFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage)
{
    if (DEBUG_TSIF) printf("ts3plugin_onClientKickFromServerEvent \n");
}

void ts3plugin_onClientIDsEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, anyID clientID, const char* clientName)
{
    if (DEBUG_TSIF) printf("ts3plugin_onClientIDsEvent\n");
}

void ts3plugin_onClientIDsFinishedEvent(uint64 serverConnectionHandlerID)
{
    if (DEBUG_TSIF) printf("ts3plugin_onClientIDsFinishedEvent\n");
}

void ts3plugin_onServerEditedEvent(uint64 serverConnectionHandlerID, anyID editerID, const char* editerName, const char* editerUniqueIdentifier)
{
}

void ts3plugin_onServerUpdatedEvent(uint64 serverConnectionHandlerID)
{
}

int ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage)
{
    if (DEBUG_TSIF) printf("PLUGIN: onServerErrorEvent %llu %s %d %s\n", (long long unsigned int)serverConnectionHandlerID, errorMessage, error, (returnCode ? returnCode : ""));
	if(returnCode)
    {
		/* A plugin could now check the returnCode with previously (when calling a function) remembered returnCodes and react accordingly */
		/* In case of using a a plugin return code, the plugin can return:
		 * 0: Client will continue handling this error (print to chat tab)
		 * 1: Client will ignore this error, the plugin announces it has handled it */
		return 1;
	}
	return 0;  /* If no plugin return code was used, the return value of this function is ignored */
}

void ts3plugin_onServerStopEvent(uint64 serverConnectionHandlerID, const char* shutdownMessage)
{
}

int ts3plugin_onTextMessageEvent(uint64 serverConnectionHandlerID, anyID targetMode, anyID toID, anyID fromID, const char* fromName, const char* fromUniqueIdentifier, const char* message, int ffIgnored)
{
    //if (DEBUG_TSIF) printf("PLUGIN: onTextMessageEvent %llu %d %d %s %s %d\n", (long long unsigned int)serverConnectionHandlerID, targetMode, fromID, fromName, message, ffIgnored);
    return 0;  /* 0 = handle normally, 1 = client will ignore the text message */
}

void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID)
{
    cPluginBase.onTalkStatusChangeEvent(serverConnectionHandlerID, status, isReceivedWhisper, clientID);
}

/* Client UI callbacks */

/*
 * Called from client when an avatar image has been downloaded to or deleted from cache.
 * This callback can be called spontaneously or in response to ts3Functions.getAvatar()
 */
void ts3plugin_onAvatarUpdated(uint64 serverConnectionHandlerID, anyID clientID, const char* avatarPath)
{
    return;
}

/*
 * Called when a plugin menu item (see ts3plugin_initMenus) is triggered. Optional function, when not using plugin menus, do not implement this.
 *
 * Parameters:
 * - serverConnectionHandlerID: ID of the current server tab
 * - type: Type of the menu (PLUGIN_MENU_TYPE_CHANNEL, PLUGIN_MENU_TYPE_CLIENT or PLUGIN_MENU_TYPE_GLOBAL)
 * - menuItemID: Id used when creating the menu item
 * - selectedItemID: Channel or Client ID in the case of PLUGIN_MENU_TYPE_CHANNEL and PLUGIN_MENU_TYPE_CLIENT. 0 for PLUGIN_MENU_TYPE_GLOBAL.
 */
void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID)
{
    cPluginBase.onMenuItemEvent(serverConnectionHandlerID, type, menuItemID, selectedItemID);
    return;
}

/* This function is called if a plugin hotkey was pressed. Omit if hotkeys are unused. */
void ts3plugin_onHotkeyEvent(const char* keyword)
{
    cPluginBase.onHotkeyEvent(keyword);
    return;
}

/* Called when recording a hotkey has finished after calling ts3Functions.requestHotkeyInputDialog */
void ts3plugin_onHotkeyRecordedEvent(const char* keyword, const char* key)
{
    return;
}

// This function receives your key Identifier you send to notifyKeyEvent and should return
// the friendly device name of the device this hotkey originates from. Used for display in UI.
const char* ts3plugin_keyDeviceName(const char* keyIdentifier)
{
	return NULL;
}

// This function translates the given key identifier to a friendly key name for display in the UI
const char* ts3plugin_displayKeyText(const char* keyIdentifier)
{
	return NULL;
}

// This is used internally as a prefix for hotkeys so we can store them without collisions.
// Should be unique across plugins.
const char* ts3plugin_keyPrefix()
{
	return NULL;
}

/* Called when client custom nickname changed */
void ts3plugin_onClientDisplayNameChanged(uint64 serverConnectionHandlerID, anyID clientID, const char* displayName, const char* uniqueClientIdentifier)
{
    return;
}
