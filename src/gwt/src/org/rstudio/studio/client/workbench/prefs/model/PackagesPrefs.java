/*
 * HistoryPrefs.java
 *
 * Copyright (C) 2009-11 by RStudio, Inc.
 *
 * This program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */
package org.rstudio.studio.client.workbench.prefs.model;

import org.rstudio.studio.client.common.cran.model.CRANMirror;

import com.google.gwt.core.client.JavaScriptObject;

public class PackagesPrefs extends JavaScriptObject
{
   protected PackagesPrefs() {}

   public static final native PackagesPrefs create(CRANMirror cranMirror) /*-{
      var prefs = new Object();
      prefs.cran_mirror = cranMirror;
      return prefs ;
   }-*/;

   public native final CRANMirror getCRANMirror() /*-{
      return this.cran_mirror;
   }-*/;   
}
