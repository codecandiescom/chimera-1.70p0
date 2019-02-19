/*
 * http.h
 *
 * Copyright (C) 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

Document *ParseHTTPResponse _ArgProto((int));
Document *ParseHTTPRequest _ArgProto((int));
Document *http _ArgProto((URLParts *, MIMEType *));
Document *http_proxy _ArgProto((URLParts *, URLParts *, MIMEType *, int));
int http_request _ArgProto((int, URLParts *, int, int));

