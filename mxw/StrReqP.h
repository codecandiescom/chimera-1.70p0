/*
 * StrReqP.h
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

#include "StrReq.h"

/************************************
 *
 *  Class structure
 *
 ***********************************/


/* New fields for the StrReq widget class record */
typedef struct _StrReqClass 
{
  XtPointer extension;
} StrReqClassPart;

/* Full class record declaration */
typedef struct _StrReqClassRec
{
  CoreClassPart           core_class;
  CompositeClassPart      composite_class;
  ShellClassPart          shell_class;
  WMShellClassPart        wm_shell_class;
  VendorShellClassPart    vendor_shell_class;
  TransientShellClassPart transient_shell_class;
  StrReqClassPart         str_req_class;
} StrReqClassRec;

extern StrReqClassRec strReqClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

/* New fields for the StrReq widget record */
typedef struct
{
  /* public */
  String defaultString;
  String message;
  Boolean hiddenText;
  XtCallbackList okCallbacks;
  XtCallbackList dismissCallbacks;

  /* private */
  Widget paned;
  Widget form;
  Widget label;
  Widget field;
  Widget box;
  Widget ok;
  Widget clear;
  Widget dismiss;
} StrReqPart;

   /* Full widget declaration */
typedef struct _StrReqRec
{
  CorePart        core;
  CompositePart   composite;
  ShellPart       shell;
  WMShellPart     wm;
  VendorShellPart vendor;
  TransientShellPart transient;
  StrReqPart      str_req;
} StrReqRec;


