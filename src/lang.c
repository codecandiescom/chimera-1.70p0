/*
 * lang.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "options.h"

#include <stdio.h>

#include "common.h"

#include "util.h"
#include "lang.h"
#include "stringdb.h"

/*
 * English/American strings for error/status messages.
 *
 * Massively rearranged for clarity.  One of those fruitless thankless
 * things, but I got fed up with not finding things in here.  Would you
 * believe that there was one duplicate entry there already? -- GN 1997Apr30
 */
static StringDB lang[] =
{
				/* Popup Prompts */
  { "enterurl",
    "Enter URL",
    lang + 1 },
  { "enterfilename",
    "Enter filename",
    lang + 2 },
  { "enterusername",
    "Enter username",
    lang + 3 },
  { "enterpw",
    "Enter password",
    lang + 4 },
  { "entersearch",
    "Enter search string",
    lang + 5 },
  { "entergroup",
    "Enter group name",
    lang + 6 },
				/* Search Prompt Page */
  { "g<index>",
    "<h1>Search</h1>Enter a search specification. <isindex>",
    lang + 7 },
				/* Popup titles */
  { "Bookmarks",
    "Bookmarks",
    lang + 8 },
				/* Status displays */
  { "crash",
    "Crash and burn on default URL.  Send email to john@cs.unlv.edu\n",
    lang + 9 },
				/* Status displays: network connections--GN */
  { "DNSquery",
    "Looking up host %s ...",
    lang + 10 },
  { "TCPconnecting",
    "Connecting to %s ...",
    lang + 11 },
  { "TCPconnected",
    "Connected to %s",
    lang + 12 },
  { "TCPsendreq",
    "Connected to %s, sending request...",
    lang + 13 },
  { "TCPsentawait",
    "Sent request, awaiting response...",
    lang + 14 },
				/* Status displays: transmission (more, GN) */
  { "download",
    "Downloading document...",
    lang + 15 },
  { "byte01",
    "Reading response: 1 byte",
    lang + 16 },
  { "byte0",
    "Reading response: %d bytes",
    lang + 17 },
  { "byte21",
    "1 byte out of %d loaded",
    lang + 18 },
  { "byte2",
    "%d bytes out of %d loaded",
    lang + 19 },
  { "byte11",
    "1 byte loaded",
    lang + 20 },
  { "byte1",
    "%d bytes loaded",
    lang + 21 },
				/* Status displays: rendering */
  { "display",
    "Displaying document...",
    lang + 22 },
				/* Error pages in rudimentary HTML */
				/* Errors: URLs... */
  { "nofirst",
    "<h1>Error</h1>Could not load the first document because the document did not exist or the URL was incorrect.",
    lang + 23 },
  { "badurl",
    "<h1>Error</h1>Invalid URL.",
    lang + 24 },
  { "emptyurl",
    "<h1>Error</h1>Empty URL.  Document not loaded.",
    lang + 25 },
  { "invalidurl",
    "<h1>Error</h1>Invalid URL.",
    lang + 26 },
  { "absurl",
    "<h1>Error</h1>Invalid URL.  You must specify an absolute URL.",
    lang + 27 },
  { "absrurl",
    "<h1>Error</h1>The proxy URL must be an absolute URL.",
    lang + 28 },
				/* Errors: HTTP, FTP,... */
  { "noload",
    "<h1>Error</h1>Could not load document:<p>",
    lang + 29 },
  { "invlocation",
    "<h1>Error</h1>Invalid relocation in reply header.",
    lang + 30 },
  { "ftperror",
    "<h1>FTP Error</h1>\n<pre>%s</pre>",
    lang + 31 },
  { "ftpweirdness",
    "<h1>Error</h1>The reply to the FTP PASV command was weird.",
    lang + 32 },
				/* Errors: HTML, rendering... */
  { "invalidaction",
    "<h1>Error</h1>Invalid ACTION (URL) in FORM.",
    lang + 33 },
  { "convfail",
    "<h1>Error</h1>Internal conversion of document failed.",
    lang + 34 },
  { "xerror",
    "<h1>X Error</h1><pre>%s</pre>",
    lang + 35 },
  { "xwarning",
    "<h1>X Warning</h1><pre>%s</pre>",
    lang + 36 },
				/* Errors: filenames, bookmarks, search... */
  { "emptyfilename",
    "<h1>Error</h1>Empty filename.  Document not saved.",
    lang + 37 },
  { "invalidfilename",
    "<h1>Error</h1>Invalid filename.  Document not saved.",
    lang + 38 },
  { "notsaved",
    "<h1>Error</h1>There was an error while the document was being saved.  Could not save document.",
    lang + 39 },
  { "nosave",
    "<h1>Error</h1>Could not save temporary file.  Make sure that you have enough temporary disk space.",
    lang + 40 },
  { "localaccess",
    "<h1>Error</h1>Could not access local file/directory.  Either the file/directory does not exist or you do not have permission to access it",
    lang + 41 },
  { "emptystring",
    "<h1>Error</h1>Empty search string.",
    lang + 42 },
  { "searchfailed",
    "<h1>Error</h1>Search failed.",
    lang + 43 },
  { "onegroup",
    "<h1>Error</h1>There must be at least one bookmark group.",
    lang + 44 },
				/* Errors: viewers and helpers... */
  { "emptyprinter",
    "<h1>Error</h1>Empty printer name.  Document not printed.",
    lang + 45 },
  { "notppipe",
    "<h1>Error</h1>Could not open a pipe to the print command.",
    lang + 46 },
  { "notpdata",
    "<h1>Error</h1>Could not send data to the print command.",
    lang + 47 },
  { "nopipe",
    "<h1>Error</h1>Could not open pipe to ",
    lang + 48 },
  { "nopipedata",
    "<h1>Error</h1>No data read from ",
    lang + 49 },
  { "noexec",
    "<h1>Error</h1>Could not execute ",
    lang + 50 },
  { "emptyemail",
    "<h1>Error</h1>Empty email address.  Email not sent.",
    lang + 51 },
  { "notepipe",
    "<h1>Error</h1>Could not open a pipe to the mail command. Email not sent.",
    lang + 52 },
  { "notedata",
    "<h1>Error</h1>Could not send data to the mail command.  Email not sent.",
    lang + 53 },
  { "notsaveext",
    "<h1>Error</h1>Could not save data for external viewer.  Check to make sure that you have enough temporary diskspace.",
    lang + 54 },
				/* Errors: inconsistent states... */
  { "nodoc",
    "<h1>Error</h1>Reached DisplayCurrent() without a document.",
    lang + 55 },
				/* Info pages */
  { "abort",
    "<h1>Info/Error</h1>The transfer was either aborted by the user or an error occurred during the download.",
    lang + 56 },
  { "needpw",
    "<h1>Info</h1>This document requires a password.",
    lang + 57 },
  { "nodata",
    "<h1>Info</h1>No data available.",
    lang + 58 },
  { "loops",
    "<h1>Info</h1>Document loops to itself.",
    lang + 59 },
  { "bookconfig",
    "<h1>Info</h1>It is possible that there is a problem with your bookmark configuration.  Bookmark list not available.",
    lang + 60 },
  { "savebook",
    "<h1>Info</h1>There was an error while the bookmark file was being written.  Make sure that you have enough diskspace to save your bookmarks.",
    lang + 61 },
  { "emptygroup",
    "<h1>Info</h1>Empty group name.  Group not added.",
    lang + 62 },
  { "nowhite",
    "<h1>Info</h1>Group names cannot have whitespace in them.",
    lang + 63 },
  { "emptybookmark",
    "<h1>Info</h1>Empty bookmark name.  Bookmark not added.",
    lang + 64 },
				/* Directory pages */
  { "ftpheader", "<title>FTP directory %s/ on %s</title>\n<h1> FTP directory %s/ </h1>\n<ul>\n",
    lang + 65 },
  { "localheader",  "<title>Local Directory %s</title>\n<h1>Local Directory %s</h1>\n<ul>\n",
    lang + 66 },
				/* Dummy list terminator */
  { "nothing", "nothing", NULL }
};

/*
 * AddLanguage
 *
 * Adds the language array or a string DB file to the string database
 */
void
AddLanguage(filename)
char *filename;
{
  FILE *fp;
  char buffer[BUFSIZ];
  char name[BUFSIZ];
  char value[BUFSIZ];

  if (filename == NULL) AddListToStringDB(lang);
  else
  {
    filename = FixFilename(filename);
    if (filename == NULL) return; /* exit quietly ... */

    fp = fopen(filename, "r");
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
      sscanf(buffer, "%[^:]:%[^\n]", name, value);
      AddToStringDB(name, value);
    }

    fclose(fp);
  }

  return;
}
