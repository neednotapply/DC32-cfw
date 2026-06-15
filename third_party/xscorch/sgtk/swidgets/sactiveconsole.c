/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/sactiveconsole.c,v 1.46 2011-07-31 19:47:59 jacob Exp $ */
/*

   xscorch - sactiveconsole.c Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Display an active console


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2 of the License ONLY.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

*/
#include <assert.h>
#include <stdlib.h>

#include <sactiveconsole.h>

#include <gdk/gdkkeysyms.h>



static ScConsoleClass *parent_class;
static ScGadgetClass  *gadget_parent_class;



enum _ScMenuConsoleSignals {
   ACTIVATE_SIGNAL,
   SELECT_SPOT_SIGNAL,
   ENTER_SPOT_SIGNAL,
   LEAVE_SPOT_SIGNAL,
   BUTTON_PRESS_SPOT_SIGNAL,
   BUTTON_RELEASE_SPOT_SIGNAL,
   KEY_PRESS_SPOT_SIGNAL,
   KEY_RELEASE_SPOT_SIGNAL,
   LAST_SIGNAL
};
static guint _sc_active_console_signals[LAST_SIGNAL] = { 0 };



enum _ScMenuGadgetSignals {
   G_PAINT_SIGNAL,
   G_SELECT_SPOT_SIGNAL,
   G_ENTER_SPOT_SIGNAL,
   G_LEAVE_SPOT_SIGNAL,
   G_BUTTON_PRESS_SPOT_SIGNAL,
   G_BUTTON_RELEASE_SPOT_SIGNAL,
   G_KEY_PRESS_SPOT_SIGNAL,
   G_KEY_RELEASE_SPOT_SIGNAL,
   G_LAST_SIGNAL
};
static guint _sc_gadget_signals[G_LAST_SIGNAL] = { 0 };



/***  GTK Custom Signal Marshallers  ***/



#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer

static void _sc_marshal_BOOLEAN__VOID(GClosure *closure, GValue *return_value,
                                      guint n_param_values, const GValue *param_values,
                                      __libj_unused gpointer invocation_hint, gpointer marshal_data) {

   typedef gint (*GMarshalFunc_BOOLEAN__VOID)(gpointer data1, gpointer data2);

   register GMarshalFunc_BOOLEAN__VOID callback;
   register GCClosure *cc = (GCClosure *)closure;
   register gpointer data1, data2;
   gboolean v_return;

   g_return_if_fail(return_value != NULL);
   g_return_if_fail(n_param_values == 1);

   if(G_CCLOSURE_SWAP_DATA(closure)) {
      data1 = closure->data;
      data2 = g_value_peek_pointer(param_values + 0);
   } else {
      data1 = g_value_peek_pointer(param_values + 0);
      data2 = closure->data;
   }
   callback = (GMarshalFunc_BOOLEAN__VOID)(marshal_data ? marshal_data : cc->callback);
   v_return = callback(data1, data2);
   g_value_set_boolean(return_value, v_return);

}



static void _sc_marshal_BOOLEAN__POINTER(GClosure *closure, GValue *return_value,
                                         guint n_param_values, const GValue *param_values,
                                         __libj_unused gpointer invocation_hint, gpointer marshal_data) {

   typedef gint (*GMarshalFunc_BOOLEAN__POINTER)(gpointer data1, gpointer arg_1,
                                                 gpointer data2);

   register GMarshalFunc_BOOLEAN__POINTER callback;
   register GCClosure *cc = (GCClosure *)closure;
   register gpointer data1, data2;
   gboolean v_return;

   g_return_if_fail(return_value != NULL);
   g_return_if_fail(n_param_values == 2);

   if(G_CCLOSURE_SWAP_DATA(closure)) {
      data1 = closure->data;
      data2 = g_value_peek_pointer(param_values + 0);
   } else {
      data1 = g_value_peek_pointer(param_values + 0);
      data2 = closure->data;
   }
   callback = (GMarshalFunc_BOOLEAN__POINTER)(marshal_data ? marshal_data : cc->callback);
   v_return = callback(data1, g_marshal_value_peek_pointer(param_values + 1), data2);
   g_value_set_boolean(return_value, v_return);

}



static void _sc_marshal_BOOLEAN__POINTER_POINTER(GClosure *closure, GValue *return_value,
                                                 guint n_param_values, const GValue *param_values,
                                                 __libj_unused gpointer invocation_hint, gpointer marshal_data) {

   typedef gint (*GMarshalFunc_BOOLEAN__POINTER_POINTER)(gpointer data1, gpointer arg_1,
                                                         gpointer arg_2, gpointer data2);

   register GMarshalFunc_BOOLEAN__POINTER_POINTER callback;
   register GCClosure *cc = (GCClosure *)closure;
   register gpointer data1, data2;
   gboolean v_return;

   g_return_if_fail(return_value != NULL);
   g_return_if_fail(n_param_values == 3);

   if(G_CCLOSURE_SWAP_DATA(closure)) {
      data1 = closure->data;
      data2 = g_value_peek_pointer(param_values + 0);
   } else {
      data1 = g_value_peek_pointer(param_values + 0);
      data2 = closure->data;
   }
   callback = (GMarshalFunc_BOOLEAN__POINTER_POINTER)(marshal_data ? marshal_data : cc->callback);
   v_return = callback(data1, g_marshal_value_peek_pointer(param_values + 1),
                       g_marshal_value_peek_pointer(param_values + 2), data2);
   g_value_set_boolean(return_value, v_return);

}



/***  Class Destructor  ***/



static void _sc_active_console_destroy(GtkObject *obj) {
/* sc_active_console_destroy
   Destroys the active console passed in.  */

   ScActiveConsole *cons = SC_ACTIVE_CONSOLE(obj);
   GList *cur = cons->spots;

   while(cur != NULL) {
      if(cur->data != NULL) {
         free(cur->data);
      }
      cur->data = NULL;
      cur = cur->next;
   }
   g_list_free(cons->spots);
   cons->spots = NULL;

   /* Call parent handler? */
   if(GTK_OBJECT_CLASS(parent_class)->destroy != NULL) {
      GTK_OBJECT_CLASS(parent_class)->destroy(obj);
   } /* Parent handler */

}



/***  Signal Emitters  ***/



static inline gboolean _sc_active_console_emit_event(ScActiveConsole *cons, int signalid,
                                                     gpointer event, ScActiveConsoleSpot *spot) {
/* sc_active_console_emit_event
   Emits the indicated signalid, which should be KEY_PRESS_SPOT_SIGNAL,
   KEY_RELEASE_SPOT_SIGNAL, BUTTON_PRESS_SPOT_SIGNAL, or
   BUTTON_RELEASE_SPOT_SIGNAL, to the spot's gadget (if one is installed),
   and then to the console itself.  If either of these signals returns
   TRUE, indicating the signal has been handled, then this function will
   return TRUE.  This function returns FALSE if both emitted signals
   return FALSE, to indicate the signal may not be fully processed.  */

   gboolean rtnval = FALSE;
   const char *signalname = NULL;

   assert(IS_SC_ACTIVE_CONSOLE(cons));
   assert(event != NULL);
   assert(spot != NULL);
   
   switch(signalid) {
   case KEY_PRESS_SPOT_SIGNAL:
      signalname = "key_press_spot";
      break;
   case KEY_RELEASE_SPOT_SIGNAL:
      signalname = "key_release_spot";
      break;
   case BUTTON_PRESS_SPOT_SIGNAL:
      signalname = "button_press_spot";
      break;
   case BUTTON_RELEASE_SPOT_SIGNAL:
      signalname = "button_release_spot";
      break;
   default:
      printf("ERROR: received bad signal ID in sactiveconsole.\n");
      abort();
   }

   /* Check to see if the spot includes a gadget. */
   if(spot->gadget != NULL) {
      assert(IS_SC_GADGET(spot->gadget));
      g_signal_emit_by_name(GTK_OBJECT(spot->gadget), signalname, event, &rtnval);
      if(rtnval) {
         return(TRUE);
      }
   }
   
   /* Emit the select-spot signal to the console itself */
   g_signal_emit(GTK_OBJECT(cons), _sc_active_console_signals[signalid], 0, 
                 spot, event, &rtnval);
   if(rtnval) {
      return(TRUE);
   }
   
   /* The signal may require additional processing if we made it here */
   return(FALSE);

}



static inline gboolean _sc_active_console_emit(ScActiveConsole *cons, int signalid,
                                               ScActiveConsoleSpot *spot) {
/* sc_active_console_emit
   Emits the indicated signalid, which should be ENTER_SPOT_SIGNAL,
   LEAVE_SPOT_SIGNAL, or SELECT_SPOT_SIGNAL, to the spot's gadget
   (if one is installed), and then to the console itself.  If either
   of these signals returns TRUE, indicating the signal has been
   handled, then this function will return TRUE.  This function
   returns FALSE if both emitted signals return FALSE, to indicate
   the signal may not be fully processed.  */

   gboolean rtnval = FALSE;
   const char *signalname = NULL;

   assert(IS_SC_ACTIVE_CONSOLE(cons));
   assert(spot != NULL);
   
   switch(signalid) {
   case ENTER_SPOT_SIGNAL:
      signalname = "enter_spot";
      break;
   case LEAVE_SPOT_SIGNAL:
      signalname = "leave_spot";
      break;
   case SELECT_SPOT_SIGNAL:
      signalname = "select_spot";
      break;
   default:
      printf("ERROR: received bad signal ID in sactiveconsole.\n");
      abort();
   }

   /* Check to see if the spot includes a gadget. */
   if(spot->gadget != NULL) {
      assert(IS_SC_GADGET(spot->gadget));
      g_signal_emit_by_name(GTK_OBJECT(spot->gadget), signalname, &rtnval);
      if(rtnval) {
         return(TRUE);
      }
   }
   
   /* Emit the select-spot signal to the console itself */
   g_signal_emit(GTK_OBJECT(cons), _sc_active_console_signals[signalid], 0, 
                 spot, &rtnval);
   if(rtnval) {
      return(TRUE);
   }
   
   /* The signal may require additional processing if we made it here */
   return(FALSE);

}



/***  Signal Handlers  ***/



static void _sc_active_console_activate(ScActiveConsole *cons) {
/* sc_active_console_activate
   Called when the console is activated with Return.  This should
   activate the current spot, if any is selected.  */

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("got an activate request%s", "");
   #endif /* debug */

   if(cons->current != NULL) {
      _sc_active_console_emit(cons, SELECT_SPOT_SIGNAL, cons->current->data);
   }

}



static inline int _sc_lines_overlap_gtk(int a1, int a2, int b1, int b2) {
/* sc_lines_overlap_gtk */

   /* Check if line segment (a1,a2) overlaps (b1,b2) */
   return(b1 <= a2 && a1 <= b2);

}



static inline int _sc_rects_overlap_gtk(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2) {
/* sc_rects_overlap_gtk */

   /* Check if rectangle A overlaps any part of rectanble B. */
   return(_sc_lines_overlap_gtk(ax1, ax2, bx1, bx2) && _sc_lines_overlap_gtk(ay1, ay2, by1, by2));

}



static void _sc_active_console_paint_region(ScConsole *_cons, GdkRectangle *bounds) {
/* _sc_active_console_paint_region
   This signal is sent to us from sconsole.c to tell us
   when we need to ask our widgets to redraw in a region. */

   ScActiveConsole *cons = SC_ACTIVE_CONSOLE(_cons);
   ScActiveConsoleSpot *spot;
   gint x1, y1, x2, y2;
   GList *cur;

   #if SC_GTK_DEBUG_PAINT
      SC_DEBUG_ENTER("paint region call%s", "");
   #endif /* debug */

   /* Check parent handler FIRST; we want to override them */
   if(parent_class->paint_region != NULL) {
      parent_class->paint_region(_cons, bounds);
   } /* Parent handler */

   /* Find our bounding box in terms of console location, not pixels. */
   x1 = bounds->x;
   y1 = bounds->y;
   x2 = bounds->x + bounds->width  - 1;
   y2 = bounds->y + bounds->height - 1;
   #if SC_GTK_DEBUG_PAINT
      SC_DEBUG_MSG("paint region is %d,%d-%d,%d CHARS", x1, y1, x2, y2);
   #endif /* debug */

   /* Look through the spots for overlapping gadgets to repaint. */
   cur = cons->spots;
   while(cur != NULL) {
      spot = cur->data;

      /* If the spot is inside the bounding box, it may need repainting. */
      if(_sc_rects_overlap_gtk(x1, y1, x2, y2,
                               spot->x, spot->y,
                               spot->x + spot->width - 1,
                               spot->y + spot->height - 1)) {
         /* Tell the gadget to repaint itself. */
         #if SC_GTK_DEBUG_PAINT
            SC_DEBUG_MSG("found a spot to repaint!  %p", spot);
         #endif /* debug */
         if(spot->gadget != NULL) {
            #if SC_GTK_DEBUG_PAINT
               SC_DEBUG_MSG("found a gadget even!  %p", spot->gadget);
            #endif /* debug */
            g_signal_emit_by_name(GTK_OBJECT(spot->gadget), "paint", NULL, NULL);
         }
      }
      cur = cur->next;
   }

}



static void _sc_active_console_enter_spot(ScActiveConsole *cons, gboolean forceenter) {

   if(!forceenter && !gtk_widget_has_focus(GTK_WIDGET(cons))) {
      return;
   }
   if(cons->current != NULL) {
      _sc_active_console_emit(cons, ENTER_SPOT_SIGNAL, cons->current->data);
   }

}



static void _sc_active_console_leave_spot(ScActiveConsole *cons, gboolean forceleave) {

   if(!forceleave && !gtk_widget_has_focus(GTK_WIDGET(cons))) {
      return;
   }
   if(cons->current != NULL) {
      _sc_active_console_emit(cons, LEAVE_SPOT_SIGNAL, cons->current->data);
   }

}



static gint _sc_active_console_focus_in(GtkWidget *widget, GdkEventFocus *focus) {

   /* Try out parent handler first */
   if(GTK_WIDGET_CLASS(parent_class)->focus_in_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->focus_in_event(widget, focus)) {
         /* Crap. The signal's already been handled */
         return(TRUE);
      } /* Signal processed? */
   } /* Signal handler available? */

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("will process focus in%s", "");
   #endif /* debug */

   _sc_active_console_enter_spot(SC_ACTIVE_CONSOLE(widget), TRUE);
   return(FALSE);

}



static gint _sc_active_console_focus_out(GtkWidget *widget, GdkEventFocus *focus) {

   /* Try out parent handler first */
   if(GTK_WIDGET_CLASS(parent_class)->focus_out_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->focus_out_event(widget, focus)) {
         /* Crap. The signal's already been handled */
         return(TRUE);
      } /* Signal processed? */
   } /* Signal handler available? */

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("will process focus out%s", "");
   #endif /* debug */

   _sc_active_console_leave_spot(SC_ACTIVE_CONSOLE(widget), TRUE);
   return(FALSE);

}



static gint _sc_active_console_key_press(GtkWidget *widget, GdkEventKey *event) {

   ScActiveConsole *cons = SC_ACTIVE_CONSOLE(widget);
   ScActiveConsoleSpot *spot;

   /* Try out parent handler first */
   if(GTK_WIDGET_CLASS(parent_class)->key_press_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->key_press_event(widget, event)) {
         /* Crap. The signal's already been handled */
         return(TRUE);
      } /* Signal processed? */
   } /* Signal handler available? */

   if(cons->current == NULL) return(FALSE);
   if(!cons->allowkeyboard) return(FALSE);

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("key is %d (%s)   xx", event->keyval, gdk_keyval_name(event->keyval));
   #endif /* debug */

   switch(event->keyval) {
      case GDK_Up:
      case GDK_KP_Up:
         _sc_active_console_leave_spot(cons, FALSE);
         cons->current = cons->current->prev;
         if(cons->current == NULL) {
            cons->current = g_list_last(cons->spots);
         }
         spot = cons->current->data;
         _sc_active_console_enter_spot(cons, FALSE);
         sc_console_set_cursor(SC_CONSOLE(cons), spot->x, spot->y, spot->width, spot->height);
         return(TRUE);

      case GDK_Down:
      case GDK_KP_Down:
         _sc_active_console_leave_spot(cons, FALSE);
         cons->current = cons->current->next;
         if(cons->current == NULL) {
            cons->current = cons->spots;
         }
         spot = cons->current->data;
         _sc_active_console_enter_spot(cons, FALSE);
         sc_console_set_cursor(SC_CONSOLE(cons), spot->x, spot->y, spot->width, spot->height);
         return(TRUE);

      /* Note: there should be NO logic here for processing GDK_Return or
         GDK_KP_Enter.  Every dialogue seems to work fine without Return
         logic here, and the tank dialogue gets really messed up if we have
         processing for Return here.  */

      case GDK_space:
      case GDK_KP_Space:
         #if SC_GTK_DEBUG_GTK && __debugging_macros
            SC_DEBUG_ENTER("emitting %d", _sc_active_console_signals[SELECT_SPOT_SIGNAL]);
         #endif /* debug */
         spot = cons->current->data;
         if(_sc_active_console_emit(cons, SELECT_SPOT_SIGNAL, spot)) {
            return(TRUE);
         }
         if(_sc_active_console_emit_event(cons, KEY_PRESS_SPOT_SIGNAL, event, spot)) {
            return(TRUE);
         }
         break;

      default:
         #if SC_GTK_DEBUG_GTK && __debugging_macros
            SC_DEBUG_ENTER("emitting %d", _sc_active_console_signals[KEY_PRESS_SPOT_SIGNAL]);
         #endif /* debug */
         spot = cons->current->data;
         if(_sc_active_console_emit_event(cons, KEY_PRESS_SPOT_SIGNAL, event, spot)) {
            return(TRUE);
         }
         break;
   }

   return(FALSE);

}



static gint _sc_active_console_key_release(GtkWidget *widget, GdkEventKey *event) {

   ScActiveConsole *cons = SC_ACTIVE_CONSOLE(widget);
   ScActiveConsoleSpot *spot;

   /* Try out parent handler first */
   if(GTK_WIDGET_CLASS(parent_class)->key_release_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->key_release_event(widget, event)) {
         /* Crap. The signal's already been handled */
         return(TRUE);
      } /* Signal processed? */
   } /* Signal handler available? */

   if(cons->current == NULL) return(FALSE);
   if(!cons->allowkeyboard) return(FALSE);

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("key is %d (%s)   xx", event->keyval, gdk_keyval_name(event->keyval));
   #endif /* debug */

   switch(event->keyval) {
      case GDK_Up:
      case GDK_KP_Up:
      case GDK_Down:
      case GDK_KP_Down:
      case GDK_space:
      case GDK_KP_Space:
         #if SC_GTK_DEBUG_GTK && __debugging_macros
            SC_DEBUG_ENTER("ignored%s", "");
         #endif /* debug */
         return(TRUE);

      default:
         #if SC_GTK_DEBUG_GTK && __debugging_macros
            SC_DEBUG_ENTER("emitting %d", _sc_active_console_signals[KEY_RELEASE_SPOT_SIGNAL]);
         #endif /* debug */
         spot = cons->current->data;
         if(_sc_active_console_emit_event(cons, KEY_RELEASE_SPOT_SIGNAL, event, spot)) {
            return(TRUE);
         }
         break;
   }

   return(FALSE);

}



/*
   This code is insane...

   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Justin go insane!
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
   All work and no play makes Jack a dull boy.
 */



static inline gboolean _sc_active_console_adjust_xy(ScActiveConsole *cons, GdkEventButton *event,
                                                    gint *row, gint *col) {
/* sc_active_console_adjust_xy */

   gint currow;
   gint curcol;

   currow = event->y;
   curcol = event->x;
   sc_console_get_char_from_pixel(SC_CONSOLE(cons), &curcol, &currow, FALSE);
   if(currow < 0 || currow >= sc_console_get_view_height(SC_CONSOLE(cons))) return(FALSE);
   if(curcol < 0 || curcol >= sc_console_get_view_width(SC_CONSOLE(cons)))  return(FALSE);

   *row = event->y;
   *col = event->x;
   sc_console_get_char_from_pixel(SC_CONSOLE(cons), col, row, TRUE);
   return(TRUE);

}



static gint _sc_active_console_button_press(GtkWidget *widget, GdkEventButton *event) {

   ScActiveConsole *cons = SC_ACTIVE_CONSOLE(widget);
   ScActiveConsoleSpot *spot;
   gint currow;
   gint curcol;
   GList *cur;

   /* Try out parent handler first */
   if(GTK_WIDGET_CLASS(parent_class)->button_press_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->button_press_event(widget, event)) {
         /* Crap. The signal's already been handled */
         return(TRUE);
      } /* Signal processed? */
   } /* Signal handler available? */

   /* Make sure this is a SINGLE click event */
   if(event->type != GDK_BUTTON_PRESS) return(FALSE);

   gtk_widget_grab_focus(GTK_WIDGET(cons));
   if(cons->current == NULL) return(FALSE);

   /* Make sure the click is within the usable window space */
   if(!_sc_active_console_adjust_xy(cons, event, &currow, &curcol)) return(FALSE);

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("will process a button press at row %d, col %d", currow, curcol);
   #endif /* debug */

   cur = cons->spots;
   while(cur != NULL) {
      spot = cur->data;
      if(curcol >= spot->x && curcol < spot->x + spot->width &&
       currow >= spot->y && currow < spot->y + spot->height) {
         _sc_active_console_leave_spot(cons, FALSE);
         cons->current = cur;
         _sc_active_console_enter_spot(cons, FALSE);
         sc_console_set_cursor(SC_CONSOLE(cons), spot->x, spot->y, spot->width, spot->height);
         sc_console_set_cursor_highlighted(SC_CONSOLE(cons), TRUE);
         _sc_active_console_emit_event(cons, BUTTON_PRESS_SPOT_SIGNAL, event, spot);
         return(TRUE);
      }
      cur = cur->next;
   }

   return(FALSE);

}



static gint _sc_active_console_button_release(GtkWidget *widget, GdkEventButton *event) {

   ScActiveConsole *cons = SC_ACTIVE_CONSOLE(widget);
   ScActiveConsoleSpot *spot;
   gint currow;
   gint curcol;
   GList *cur;

   /* Try out parent handler first */
   if(GTK_WIDGET_CLASS(parent_class)->button_release_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->button_release_event(widget, event)) {
         /* Crap. The signal's already been handled */
         return(TRUE);
      } /* Signal processed? */
   } /* Signal handler available? */

   gtk_widget_grab_focus(GTK_WIDGET(cons));
   if(cons->current == NULL) return(FALSE);

   /* Adjust click so it is within the usable window space */
   _sc_active_console_adjust_xy(cons, event, &currow, &curcol);

   #if SC_GTK_DEBUG_GTK && __debugging_macros
      SC_DEBUG_ENTER("will process a button release at row %d, col %d", currow, curcol);
   #endif /* debug */

   spot = cons->current->data;
   if(cons->allowkeyboard) {
      /* Maintain the cursor, just unhighlight it */
      sc_console_set_cursor(SC_CONSOLE(cons), spot->x, spot->y, spot->width, spot->height);
      sc_console_set_cursor_highlighted(SC_CONSOLE(cons), FALSE);
   } else {
      /* No keyboard allowed; just destroy the cursor */
      sc_console_set_cursor(SC_CONSOLE(cons), 0, 0, 0, 0);
      sc_console_set_cursor_highlighted(SC_CONSOLE(cons), FALSE);
   }

   cur = cons->spots;
   while(cur != NULL) {
      spot = cur->data;
      if(curcol >= spot->x && curcol < spot->x + spot->width &&
         currow >= spot->y && currow < spot->y + spot->height) {
         if(cons->current == cur) {
            if(_sc_active_console_emit(cons, SELECT_SPOT_SIGNAL, spot)) {
               return(TRUE);
            }
         }
         _sc_active_console_emit_event(cons, BUTTON_RELEASE_SPOT_SIGNAL, event, spot);
         return(TRUE);
      }
      cur = cur->next;
   }

   return(FALSE);

}



static void _sc_active_console_class_init(ScActiveConsoleClass *klass) {

   GtkObjectClass *object_class = (GtkObjectClass *)klass;

   parent_class = g_type_class_peek(sc_console_get_type());

   _sc_active_console_signals[ACTIVATE_SIGNAL] =
      g_signal_new("activate",                     /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                                   /* Signal flags; run this signal first */
                   offsetof(ScActiveConsoleClass, activate),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   g_cclosure_marshal_VOID__VOID,
                                                   /* Marshal function for this signal */
                   G_TYPE_NONE,                    /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );
   _sc_active_console_signals[SELECT_SPOT_SIGNAL] =
      g_signal_new("select_spot",                  /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScActiveConsoleClass, select_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER,   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   1,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER                  /* Type of first argument */
                  );
   _sc_active_console_signals[ENTER_SPOT_SIGNAL] =
      g_signal_new("enter_spot",                   /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScActiveConsoleClass, enter_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER,   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   1,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER                  /* Type of first argument */
                  );
   _sc_active_console_signals[LEAVE_SPOT_SIGNAL] =
      g_signal_new("leave_spot",                   /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScActiveConsoleClass, leave_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER,   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   1,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER                  /* Type of first argument */
                  );
   _sc_active_console_signals[BUTTON_PRESS_SPOT_SIGNAL] =
      g_signal_new("button_press_spot",            /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScActiveConsoleClass, button_press_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER_POINTER,
                                                   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   2,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER,                 /* Type of first argument */
                   G_TYPE_POINTER                  /* Type of second argument */
                  );
   _sc_active_console_signals[BUTTON_RELEASE_SPOT_SIGNAL] =
      g_signal_new("button_release_spot",          /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScActiveConsoleClass, button_release_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER_POINTER,
                                                   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   2,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER,                 /* Type of first argument */
                   G_TYPE_POINTER                  /* Type of second argument */
                  );
   _sc_active_console_signals[KEY_PRESS_SPOT_SIGNAL] =
      g_signal_new("key_press_spot",               /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScActiveConsoleClass, key_press_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER_POINTER,
                                                   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   2,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER,                 /* Type of first argument */
                   G_TYPE_POINTER                  /* Type of second argument */
                  );
   _sc_active_console_signals[KEY_RELEASE_SPOT_SIGNAL] =
      g_signal_new("key_release_spot",             /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScActiveConsoleClass, key_release_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER_POINTER,
                                                   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   2,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER,                 /* Type of first argument */
                   G_TYPE_POINTER                  /* Type of second argument */
                  );

   ((GtkWidgetClass *)klass)->activate_signal = _sc_active_console_signals[ACTIVATE_SIGNAL];

   /* Setup new signal default handlers */
   klass->activate            = _sc_active_console_activate;
   klass->select_spot         = NULL;
   klass->enter_spot          = NULL;
   klass->leave_spot          = NULL;
   klass->button_press_spot   = NULL;
   klass->button_release_spot = NULL;
   klass->key_press_spot      = NULL;
   klass->key_release_spot    = NULL;

   /* Install signals from parent class */
   SC_CONSOLE_CLASS(klass)->paint_region        = _sc_active_console_paint_region;
   GTK_WIDGET_CLASS(klass)->key_press_event     = _sc_active_console_key_press;
   GTK_WIDGET_CLASS(klass)->key_release_event   = _sc_active_console_key_release;
   GTK_WIDGET_CLASS(klass)->button_press_event  = _sc_active_console_button_press;
   GTK_WIDGET_CLASS(klass)->button_release_event= _sc_active_console_button_release;
   GTK_WIDGET_CLASS(klass)->focus_in_event      = _sc_active_console_focus_in;
   GTK_WIDGET_CLASS(klass)->focus_out_event     = _sc_active_console_focus_out;
   GTK_OBJECT_CLASS(klass)->destroy             = _sc_active_console_destroy;

}



static void _sc_gadget_class_init(ScGadgetClass *klass) {

   GtkObjectClass *object_class = (GtkObjectClass *)klass;

   gadget_parent_class = g_type_class_peek(gtk_object_get_type());

   _sc_gadget_signals[G_PAINT_SIGNAL] =
      g_signal_new("paint",                        /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScGadgetClass, paint),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   g_cclosure_marshal_VOID__VOID,
                                                   /* Marshal function for this signal */
                   G_TYPE_NONE,                    /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );
   _sc_gadget_signals[G_SELECT_SPOT_SIGNAL] =
      g_signal_new("select_spot",                  /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScGadgetClass, select_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__VOID,      /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );
   _sc_gadget_signals[G_ENTER_SPOT_SIGNAL] =
      g_signal_new("enter_spot",                   /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScGadgetClass, enter_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__VOID,      /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );
   _sc_gadget_signals[G_LEAVE_SPOT_SIGNAL] =
      g_signal_new("leave_spot",                   /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScGadgetClass, leave_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__VOID,      /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );
   _sc_gadget_signals[G_BUTTON_PRESS_SPOT_SIGNAL] =
      g_signal_new("button_press_spot",            /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScGadgetClass, button_press_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER,   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   1,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER                  /* Type of first argument */
                  );
   _sc_gadget_signals[G_BUTTON_RELEASE_SPOT_SIGNAL] =
      g_signal_new("button_release_spot",          /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScGadgetClass, button_release_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER,   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   1,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER                  /* Type of first argument */
                  );
   _sc_gadget_signals[G_KEY_PRESS_SPOT_SIGNAL] =
      g_signal_new("key_press_spot",               /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScGadgetClass, key_press_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER,   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   1,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER                  /* Type of first argument */
                  );
   _sc_gadget_signals[G_KEY_RELEASE_SPOT_SIGNAL] =
      g_signal_new("key_release_spot",             /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScGadgetClass, key_release_spot),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   _sc_marshal_BOOLEAN__POINTER,   /* Marshal function for this signal */
                   G_TYPE_BOOLEAN,                 /* Return type for the marshaller */
                   1,                              /* Number of extra parametres to pass */
                   G_TYPE_POINTER                  /* Type of first argument */
                  );

   /* Setup new signal default handlers */
   klass->paint               = NULL;
   klass->select_spot         = NULL;
   klass->enter_spot          = NULL;
   klass->leave_spot          = NULL;
   klass->button_press_spot   = NULL;
   klass->button_release_spot = NULL;
   klass->key_press_spot      = NULL;
   klass->key_release_spot    = NULL;

}



static void _sc_active_console_init_obj(ScActiveConsole *cons) {

   cons->spots = NULL;
   cons->current = NULL;

   gtk_widget_set_can_default(GTK_WIDGET(cons), TRUE);
   gtk_widget_set_events(GTK_WIDGET(cons), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

}



static void _sc_gadget_init_obj(ScGadget *gadget) {

   gadget->spot = NULL;
   gadget->console = NULL;
   gadget->x = 0;
   gadget->y = 0;
   gadget->width = 0;
   gadget->height = 0;

}



GType sc_active_console_get_type(void) {

   static GType sc_active_console_type = 0;

   if(sc_active_console_type == 0) {
      static const GTypeInfo sc_active_console_info = {
         sizeof(ScActiveConsoleClass),    /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         (GClassInitFunc)_sc_active_console_class_init,
                                          /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScActiveConsole),         /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_active_console_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_active_console_type = g_type_register_static(sc_console_get_type(), "ScActiveConsole",
                                                      &sc_active_console_info, 0);
   }

   return(sc_active_console_type);

}



GType sc_gadget_get_type(void) {

   static GType sc_gadget_type = 0;

   if(sc_gadget_type == 0) {
      static const GTypeInfo sc_gadget_info = {
         sizeof(ScGadgetClass),           /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         (GClassInitFunc)_sc_gadget_class_init,
                                          /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScGadget),                /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_gadget_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_gadget_type = g_type_register_static(gtk_object_get_type(), "ScGadget",
                                              &sc_gadget_info, 0);
   }

   return(sc_gadget_type);

}



void sc_active_console_init(ScActiveConsole *cons, gint x, gint y, gint width, gint height, ScConsoleStyle style,
                            GdkFont *font, GdkFont *boldfont) {

   cons->current = NULL;
   sc_console_init(SC_CONSOLE(cons), x, y, width, height, style, font, boldfont);
   sc_console_set_cursor(SC_CONSOLE(cons), 0, 0, 0, 0);
   gtk_widget_set_sensitive(GTK_WIDGET(cons), TRUE);

}



GtkWidget *sc_active_console_new(gint x, gint y, gint width, gint height, ScConsoleStyle style,
                                 GdkFont *font, GdkFont *boldfont) {

   ScActiveConsole *cons;

   cons = g_object_new(sc_active_console_get_type(), NULL);
   g_return_val_if_fail(cons != NULL, NULL);

   cons->allowkeyboard = TRUE;
   gtk_widget_set_sensitive(GTK_WIDGET(cons), TRUE);

   sc_active_console_init(cons, x, y, width, height, style, font, boldfont);
   return(GTK_WIDGET(cons));

}



static ScActiveConsoleSpot *_sc_active_console_new_spot(gint x, gint y, gint width, gint height, gpointer data) {

   ScActiveConsoleSpot *spot;

   spot = (ScActiveConsoleSpot *)malloc(sizeof(ScActiveConsoleSpot));
   if(spot == NULL) return(NULL);

   spot->x = x;
   spot->y = y;
   spot->width = width;
   spot->height= height;
   spot->data  = data;
   spot->gadget = NULL;

   return(spot);

}



static void _sc_active_console_append_spot(ScActiveConsole *cons, ScActiveConsoleSpot *spot) {

   if(spot == NULL) return;
   cons->spots = g_list_append(cons->spots, spot);
   if(cons->current == NULL) {
      cons->current = cons->spots;
      _sc_active_console_enter_spot(cons, FALSE);
      if(cons->allowkeyboard) {
         sc_console_set_cursor(SC_CONSOLE(cons), spot->x, spot->y, spot->width, spot->height);
      }
   }

}



void sc_active_console_add_spot(ScActiveConsole *cons, gint x, gint y, gint width, gint height, gpointer data) {

   ScActiveConsoleSpot *spot;
   spot = _sc_active_console_new_spot(x, y, width, height, data);
   _sc_active_console_append_spot(cons, spot);

}



void sc_active_console_add_row_spot(ScActiveConsole *cons, gint row, gpointer data) {

   sc_active_console_add_spot(cons, 0, row, sc_console_get_width(SC_CONSOLE(cons)), 1, data);

}



void sc_active_console_add_gadget_spot(ScActiveConsole *cons, ScGadget *gadget, gpointer data) {

   ScActiveConsoleSpot *spot;
   if(gadget == NULL) return;
   spot = _sc_active_console_new_spot(gadget->x, gadget->y, gadget->width, gadget->height, data);
   if(spot != NULL) {
      /* what the gadget knows about its spot */
      gadget->spot = spot;
      gadget->console = cons;

      /* what the spot knows about its gadget */
      spot->gadget = gadget;
      _sc_active_console_append_spot(cons, spot);
   }

}



gboolean sc_active_console_detach_spot(ScActiveConsole *cons) {

   ScActiveConsoleSpot *spot;
   GList *cur;
   gint x;
   gint y;
   gint width;
   gint height;

   /* Is there anything to delete? */
   cur = cons->spots;
   if(cur == NULL) return(FALSE);

   /* reset the current selection */
   cons->current = NULL;
   _sc_active_console_leave_spot(cons, FALSE);
   sc_console_set_cursor(SC_CONSOLE(cons), 0, 0, 0, 0);
   sc_console_set_cursor_highlighted(SC_CONSOLE(cons), FALSE);

   /* Delete the last spot */
   while(cur->next != NULL) cur = cur->next;
   spot = cur->data;
   cur->data = NULL;

   /* Release any associated gadget */
   if(spot->gadget != NULL) {
      g_object_unref(GTK_OBJECT(spot->gadget));
      spot->gadget = NULL;
   }

   x = spot->x;
   y = spot->y;
   width = spot->width;
   height= spot->height;

   free(cur->data);
   cur->data = NULL;
   cons->spots = g_list_remove_link(cons->spots, cur);
   g_list_free(cur);

   return(TRUE);

}



void sc_active_console_detach_all_spots(ScActiveConsole *cons) {

   while(sc_active_console_detach_spot(cons)) /* Just loop */;

}



void sc_active_console_set_allow_keyboard(ScActiveConsole *cons, gboolean allowkeyboard) {

   cons->allowkeyboard = allowkeyboard;

}



void sc_gadget_get_extents(ScGadget *gadget, GdkRectangle *bounds) {

   ScConsole *cons;
   gint x, y;

   if(bounds == NULL) return;
   bounds->width = bounds->height = 0;

   if(gadget == NULL) return;
   if(gadget->spot == NULL) return;
   if(gadget->console == NULL) return;
   cons = SC_CONSOLE(gadget->console);

   /* Determine the pixel locations. */
   x = gadget->spot->x;
   y = gadget->spot->y;
   sc_console_get_pixel_from_char(cons, &x, &y, TRUE);

   /* Update the result rectangle */
   bounds->x      = x;
   bounds->y      = y;
   bounds->width  = sc_console_get_col_width (cons) * gadget->spot->width;
   bounds->height = sc_console_get_row_height(cons) * gadget->spot->height;

}
