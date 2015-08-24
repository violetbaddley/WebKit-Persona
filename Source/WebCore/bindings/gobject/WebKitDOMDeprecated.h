/*
 *  Copyright (C) 2014 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef WebKitDOMDeprecated_h
#define WebKitDOMDeprecated_h

#if !defined(WEBKIT_DISABLE_DEPRECATED)

#include <glib.h>
#include <webkitdom/webkitdomdefines.h>

G_BEGIN_DECLS

/**
 * webkit_dom_html_element_get_inner_html:
 * @self: a #WebKitDOMHTMLElement
 *
 * Returns: a #gchar
 *
 * Deprecated: 2.8: Use webkit_dom_element_get_inner_html() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_element_get_inner_html) gchar*
webkit_dom_html_element_get_inner_html(WebKitDOMHTMLElement* self);

/**
 * webkit_dom_html_element_set_inner_html:
 * @self: a #WebKitDOMHTMLElement
 * @contents: a #gchar with contents to set
 * @error: a #GError or %NULL
 *
 * Deprecated: 2.8: Use webkit_dom_element_set_inner_html() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_element_set_inner_html) void
webkit_dom_html_element_set_inner_html(WebKitDOMHTMLElement* self, const gchar* contents, GError** error);

/**
 * webkit_dom_html_element_get_outer_html:
 * @self: a #WebKitDOMHTMLElement
 *
 * Returns: a #gchar
 *
 * Deprecated: 2.8: Use webkit_dom_element_get_outer_html() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_element_get_outer_html) gchar*
webkit_dom_html_element_get_outer_html(WebKitDOMHTMLElement* self);

/**
 * webkit_dom_html_element_set_outer_html:
 * @self: a #WebKitDOMHTMLElement
 * @contents: a #gchar with contents to set
 * @error: a #GError or %NULL
 *
 * Deprecated: 2.8: Use webkit_dom_element_set_outer_html() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_element_set_outer_html) void
webkit_dom_html_element_set_outer_html(WebKitDOMHTMLElement* self, const gchar* contents, GError** error);

/**
 * webkit_dom_html_element_get_children:
 * @self: A #WebKitDOMHTMLElement
 *
 * Returns: (transfer full): A #WebKitDOMHTMLCollection
 *
 * Deprecated: 2.10: Use webkit_dom_element_get_children() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_element_get_children) WebKitDOMHTMLCollection*
webkit_dom_html_element_get_children(WebKitDOMHTMLElement* self);

/**
 * webkit_dom_document_get_elements_by_tag_name:
 * @self: A #WebKitDOMDocument
 * @tag_name: a #gchar with the tag name
 *
 * Returns: (transfer full): a #WebKitDOMNodeList
 *
 * Deprecated: 2.12: Use webkit_dom_document_get_elements_by_tag_name_as_html_collection() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_document_get_elements_by_tag_name_as_html_collection) WebKitDOMNodeList*
webkit_dom_document_get_elements_by_tag_name(WebKitDOMDocument* self, const gchar* tag_name);

/**
 * webkit_dom_document_get_elements_by_tag_name_ns:
 * @self: A #WebKitDOMDocument
 * @namespace_uri: a #gchar with the namespace URI
 * @tag_name: a #gchar with the tag name
 *
 * Returns: (transfer full): a #WebKitDOMNodeList
 *
 * Deprecated: 2.12: Use webkit_dom_document_get_elements_by_tag_name_ns_as_html_collection() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_document_get_elements_by_tag_name_as_html_collection) WebKitDOMNodeList*
webkit_dom_document_get_elements_by_tag_name_ns(WebKitDOMDocument* self, const gchar* namespace_uri, const gchar* tag_name);


/**
 * webkit_dom_document_get_elements_by_class_name:
 * @self: A #WebKitDOMDocument
 * @class_name: a #gchar with the tag name
 *
 * Returns: (transfer full): a #WebKitDOMNodeList
 *
 * Deprecated: 2.12: Use webkit_dom_document_get_elements_by_class_name_as_html_collection() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_document_get_elements_by_class_name_as_html_collection) WebKitDOMNodeList*
webkit_dom_document_get_elements_by_class_name(WebKitDOMDocument* self, const gchar* class_name);

/**
 * webkit_dom_element_get_elements_by_tag_name:
 * @self: A #WebKitDOMElement
 * @tag_name: a #gchar with the tag name
 *
 * Returns: (transfer full): a #WebKitDOMNodeList
 *
 * Deprecated: 2.12: Use webkit_dom_element_get_elements_by_tag_name_as_html_collection() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_element_get_elements_by_tag_name_as_html_collection) WebKitDOMNodeList*
webkit_dom_element_get_elements_by_tag_name(WebKitDOMElement* self, const gchar* tag_name);

/**
 * webkit_dom_element_get_elements_by_tag_name_ns:
 * @self: A #WebKitDOMElement
 * @namespace_uri: a #gchar with the namespace URI
 * @tag_name: a #gchar with the tag name
 *
 * Returns: (transfer full): a #WebKitDOMNodeList
 *
 * Deprecated: 2.12: Use webkit_dom_element_get_elements_by_tag_name_ns_as_html_collection() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_element_get_elements_by_tag_name_as_html_collection) WebKitDOMNodeList*
webkit_dom_element_get_elements_by_tag_name_ns(WebKitDOMElement* self, const gchar* namespace_uri, const gchar* tag_name);


/**
 * webkit_dom_element_get_elements_by_class_name:
 * @self: A #WebKitDOMElement
 * @class_name: a #gchar with the tag name
 *
 * Returns: (transfer full): a #WebKitDOMNodeList
 *
 * Deprecated: 2.12: Use webkit_dom_element_get_elements_by_class_name_as_html_collection() instead.
 */
WEBKIT_DEPRECATED_FOR(webkit_dom_element_get_elements_by_class_name_as_html_collection) WebKitDOMNodeList*
webkit_dom_element_get_elements_by_class_name(WebKitDOMElement* self, const gchar* class_name);

G_END_DECLS

#endif /* WEBKIT_DISABLE_DEPRECATED */

#endif
