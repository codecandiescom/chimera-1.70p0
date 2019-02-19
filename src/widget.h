/*
 * widget.h
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */ 

typedef struct _realminfo
{
  char *name;
  URLParts *up;
  int used;
  struct _realminfo *next;
} RealmInfo;

typedef struct _docnode
{
  Document *doc;
  int vpos, hpos;
  URLParts *up;
  char *base;
  struct _docnode *next;
} DocNode;

typedef struct _htmlroot
{
  /*
   * Public
   */
  XtTranslations trans;
  char *keyTrans;
  char *path;
  char *convertFiles;
  char *mailCapFiles;
  char *protocolFiles;
  char *mimeTypeFiles;
  char *bookmarkFile;
  char *languageDB;
  char *cacheInfoFiles;
  int cacheTTL;
  int cacheSize;
  Boolean cacheOff;
  char *cacheDir;
  Boolean cacheClean;
  char *helpURL; /* location of the help page */
  char *homeURL; /* location of the home page */
  char *button1Box; /* list of widgets in the first button box */
  char *button2Box; /* list of widgets in the second button box */
  char *printerName; /* default printer */
  Boolean showURL; /* switch for the display of the current URL */
  Boolean showTitle; /* switch for the display of the current title */
  Boolean anchorDisplay; /* display URL of current hyperlink */
  int statusUpdate; /* frequency of download status update */
  int inPort; /* the port that chimera listens on for data */
  char *httpProxy;
  char *gopherProxy;
  char *ftpProxy;
  char *waisProxy;
  char *nntpProxy;
  char *newsProxy;
  char *urnProxy;
  char *noProxy;
  char *allProxy;
  char *email;
  float gamma;
  int maxColors; /* maximum colors per inline image */
  Boolean cacheIgnoreExpires;
  Boolean openButtonShortcut; /* enable foo.com -> http://foo.com/ shortcut */
  char *localIndexFiles; /* look for index.html etc. in local directories */

  /*
   * Private data
   */
  Boolean rflag; /* reload flag */
  RealmInfo *rlist; /* list of realm authentication info */
  XColor bgcolor; /* background color */
  XtAppContext appcon;
  Widget file;
  Widget toplevel;
  Widget w;
  Widget back;
  Widget load;
  Widget view;
  Widget help;
  Widget source;
  Widget urldisplay;
  Widget titledisplay;
  Widget bookmark;
  Widget reload;
  Widget home;
  Widget search;
  Widget cancel;
  Widget quit;
  Widget deferpix;	       /* WBE */
  Widget anchordisplay;        /* rwmcm */
  char *savestr;
  char *loadstr;
  char *printstr;
  char *searchstr;
  char *mailstr;
  char *base;
  int otype;
  int ttype;
  Boolean cancelop;
  DocNode *dlist;
  Cursor left_ptr;
  Cursor watch;
  Convert *clist;
  MailCap *mclist;
  MIMEType *mtlist;
  Protocol *plist;
  String group;
} AppResources;

/*
 * Callbacks
 *
 * It would be a pain to put args for these guys because I would
 * have to do funky things with the middle argument.
 */
void Quit();
void OpenDocument();
void Anchor();
void AnchorURLDisplay();
void Home();
void Back();
void Help();
void Source();
void Reload();
void File();
void FileAction();
void OpenAction();
void SearchAction();
void BookmarkAction();
void Search();
void Cancel();
void DeferPix();
ImageInfo *ImageResolve();
ImageInfo *DelayedImageResolve();
int VisitTest();
void SubmitForm();
void LinkCB();

void CreateWidgets _ArgProto((AppResources *));



