/*
 * OutputSelP.h
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

#include "OutputSel.h"

/************************************
 *
 *  Class structure
 *
 ***********************************/


/* New fields for the OutputSel widget class record */
typedef struct _OutputSelClass 
{
  XtPointer extension;
} OutputSelClassPart;

/* Full class record declaration */
typedef struct _OutputSelClassRec
{
  CoreClassPart           core_class;
  CompositeClassPart      composite_class;
  ShellClassPart          shell_class;
  WMShellClassPart        wm_shell_class;
  VendorShellClassPart    vendor_shell_class;
  TransientShellClassPart transient_shell_class;
  OutputSelClassPart       OutputSel_class;
} OutputSelClassRec;

extern OutputSelClassRec outputSelClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

/* New fields for the OutputSel widget record */
typedef struct
{
  /* public */
  String defaultPrinter;
  String defaultEmail;
  String defaultFilename;
  XtCallbackList okCallbacks;
  XtCallbackList dismissCallbacks;
  int outputType;
  int outputDevice;

  /* private */
  Widget paned;
  Widget tbox;
  Widget types[4];
  Widget obox;
  Widget outs[3];
  Widget form;
  Widget field;
  Widget box;
  Widget ok;
  Widget clear;
  Widget dismiss;
} OutputSelPart;

   /* Full widget declaration */
typedef struct _OutputSelRec
{
  CorePart        core;
  CompositePart   composite;
  ShellPart       shell;
  WMShellPart     wm;
  VendorShellPart vendor;
  TransientShellPart transient;
  OutputSelPart   output_sel;
} OutputSelRec;


