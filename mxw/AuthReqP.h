/*
 * AuthReqP.h
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

#include "AuthReq.h"

/************************************
 *
 *  Class structure
 *
 ***********************************/


/* New fields for the AuthReq widget class record */
typedef struct _AuthReqClass 
{
  XtPointer extension;
} AuthReqClassPart;

/* Full class record declaration */
typedef struct _AuthReqClassRec
{
  CoreClassPart           core_class;
  CompositeClassPart      composite_class;
  ShellClassPart          shell_class;
  WMShellClassPart        wm_shell_class;
  VendorShellClassPart    vendor_shell_class;
  TransientShellClassPart transient_shell_class;
  AuthReqClassPart         auth_req_class;
} AuthReqClassRec;

extern AuthReqClassRec authReqClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

/* New fields for the AuthReq widget record */
typedef struct
{
  /* public */
  String defaultUsername;
  String message;
  XtCallbackList okCallbacks;
  XtCallbackList dismissCallbacks;

  /* private */
  Widget paned;
  Widget uform;
  Widget pform;
  Widget ulabel;
  Widget plabel;
  Widget username;
  Widget password;
  Widget box;
  Widget ok;
  Widget clear;
  Widget dismiss;
} AuthReqPart;

   /* Full widget declaration */
typedef struct _AuthReqRec
{
  CorePart        core;
  CompositePart   composite;
  ShellPart       shell;
  WMShellPart     wm;
  VendorShellPart vendor;
  TransientShellPart transient;
  AuthReqPart      auth_req;
} AuthReqRec;


