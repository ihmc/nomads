/*
 * XmlProcessor.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Created on November 21, 2006, 10:05 AM
 *
 */

package us.ihmc.util.xml;

import org.dom4j.Document;
import org.dom4j.DocumentHelper;
import org.dom4j.Element;
import org.dom4j.Namespace;
import org.dom4j.Node;
import org.dom4j.XPath;
import org.dom4j.io.OutputFormat;
import org.dom4j.io.SAXReader;
import org.dom4j.io.XMLWriter;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;


/**
 *
 * @author sstabellini
 */
public class XmlProcessor
{
  
  
  public XmlProcessor(String doc)
  {
    SAXReader reader = new SAXReader();
    Node node = null;

    byte[] docBytes = doc.getBytes();
    ByteArrayInputStream docStream = new ByteArrayInputStream(docBytes);

    try
    {
      document = reader.read(docStream);
    }
    catch(Exception e)
    {
      e.printStackTrace();
    }
  }


  public synchronized List findElementValues(String context, String tagName)
  {
    XPath select = null;
    String xpath = null;

    if(context == null)
    {
      if (tagName == null)
        return null;
      else xpath = "//"+tagName;
    }
    else
    {
      if(tagName == null) xpath = context;
      else xpath = context+"/"+tagName;
    }

    select = DocumentHelper.createXPath(xpath);

    List filter = select.selectNodes(document, select);

    for( Iterator iterA = filter.iterator(); iterA.hasNext(); )
    {
      Element element = (Element) iterA.next();
    }
    return filter;
  }

  public synchronized List findElementValues(String path)
  {
    XPath select = null;
    String xpath = null;

    select = DocumentHelper.createXPath(path);

    List filter = select.selectNodes(document, select);
    return filter;
  }

  public synchronized Element addTextElement(Element elem, String text)
  {
    elem.setText(text);

    return elem;
  }

  public synchronized void createTextElement(String path, String text)
  {
    List nodes = null;
    Element elem = null;
    Element newElem = DocumentHelper.makeElement(document, path);
    
    nodes = this.findElementValues(path);
    elem = (Element) nodes.get(0);
    elem.setText(text);
  }
  
  public synchronized void createTextElement(Document doc, String path, String text)
  {
    List nodes = null;
    Element elem = null;
    Element newElem = DocumentHelper.makeElement(doc, path);
    
    nodes = this.findElementValues(path);
    elem = (Element) nodes.get(0);
    elem.setText(text);
  }
  
  public synchronized Element insertElementAt(Element currentElement, Element newElement, int index) 
  {
    Element parent = null;
    
    if(!currentElement.isRootElement())
    {
        parent = currentElement.getParent();
    }
    else parent = currentElement;
    
    List list = parent.content();
    list.add(index, newElement);
    
    return currentElement;
  }

  public synchronized Element insertElementUnderRoot(Element newElement, int index) 
  {
    Element root = document.getRootElement();
    
    List list = root.content();
    list.add(index, newElement);
    
    return root;
  }
   
  
  public synchronized Element removeNodes(Element src, Vector paths)
  {
    Vector nodes = new Vector();
    // create a Vector of Nodes using XPath
    Iterator pathIter = paths.iterator();
    
    while(pathIter.hasNext())
    {
        String path = (String) pathIter.next();
        List nodeList = src.selectNodes(path);
        
        for(Iterator iter = nodeList.iterator(); iter.hasNext();)
        {
            Node n = (Node) iter.next();
            n.detach();
        }
    }
    
    return src;
  }
  

  public synchronized Element removeNodesUnderRoot(Vector paths)
  {
    Element root = document.getRootElement();
    Vector nodes = new Vector();
    // create a Vector of Nodes using XPath
    Iterator pathIter = paths.iterator();
    
    while(pathIter.hasNext())
    {
        String path = (String) pathIter.next();
        List nodeList = root.selectNodes(path);
        
        for(Iterator iter = nodeList.iterator(); iter.hasNext();)
        {
            Node n = (Node) iter.next();
            n.detach();
        }
    }
    
    return root;
  }

  
  public synchronized Element removeNode(Element src, String path)
  {

    List nodeList = src.selectNodes(path);

    for(Iterator iter = nodeList.iterator(); iter.hasNext();)
    {
        Node n = (Node) iter.next();
        n.detach();
    }

    
    return src;
  }  

  
  public synchronized Element removeNodeUnderRoot(String path)
  {

    Element root = document.getRootElement();
    List nodeList = root.selectNodes(path);

    for(Iterator iter = nodeList.iterator(); iter.hasNext();)
    {
        Node n = (Node) iter.next();
        n.detach();
    }

    
    return root;
  } 
  
  
  public synchronized void createTextElement(String path)
  {
    Element newElem = DocumentHelper.makeElement(document, path);
  }  
  
  
  
  public synchronized String addTextElement(String path, String text, boolean force)
  {
    List nodes = this.findElementValues(path);
    Element elem = null;

    if(!nodes.isEmpty())
    {
      elem = (Element) nodes.get(0);
      elem.setText(text);
    }
    else
    {
      if(force)
      {
        this.createTextElement(path, text);
      }
      else elem = null;
    }

    return document.asXML();
  }
  
  public synchronized String addTextElement(String parentPath, String elementName, String text)
  {
    List nodes = this.findElementValues(parentPath);
    
    Element elem = null;
    
    elem = (Element) nodes.get(0);
    
    Element newElem = elem.addElement(elementName);
    newElem.setText(text);
    
    return document.asXML();
  }
  

  public synchronized Document createDocument(String doc)
  {
    Document DOM = null;
    SAXReader reader = new SAXReader();

    byte[] docBytes = doc.getBytes();
    ByteArrayInputStream docStream = new ByteArrayInputStream(docBytes);

    try
    {
      DOM = reader.read(docStream);
    }
    catch(Exception e)
    {
        e.printStackTrace();
    }
    
    return DOM;
  }
  
  
  // all nodes contained within the subelement identified by XPath expression path within src node
  // are added at the end of node list contained within dst. this method assumes first hit in src given
  // XPath expression will be used.
  public synchronized Element addSubTree(Element src, Element dst, String path)
  {
    List elements = DocumentHelper.selectNodes(path, src);
    Element unique = null;
    
    if(elements != null) unique = (Element) elements.get(0);
    else return src;

    // get all nodes contained within unique
    List contents = unique.selectNodes("*");
 
    Iterator iter = contents.iterator();
    while(iter.hasNext())
    {
        Node n = ((Element) iter.next()).detach();
        dst.add(n);
    }

    return dst;
  }
  
  
  public synchronized boolean exists(String path)
  {
    List nodes = this.findElementValues(path);
    if(nodes.isEmpty()) return false;
    else return true;
  }
  
  
  public synchronized boolean exists(Element elem, String path)
  {
    List nodes = elem.selectNodes(path);
    
    if(nodes.isEmpty()) return false;
    else return true;
  }
  
  
  public synchronized void getNamespaceURIs(Element elem, HashMap uris)
  {
    // walk children and call recursively with each child element
    Iterator children = elem.elementIterator();
  
    while(children.hasNext())
    {
      Element child = (Element) children.next();
      this.getNamespaceURIs(child, uris);
    }
    
    // finished so process information associated with THIS element
    Namespace elemNS = elem.getNamespace();
    String tag = elemNS.getPrefix();
    String uri = elemNS.getURI();
    
    if(tag!="" && uri!="") uris.put(tag, uri);
    
    // now snag the additional namespaces
    List additional = elem.additionalNamespaces();
    int numNSs = additional.size();
    
    for(int i=0; i<numNSs; i++)
    {  
      Namespace addNS = (Namespace) additional.get(i);
      
      tag = addNS.getPrefix();
      uri = addNS.getURI();

      if(tag!="" && uri!="") uris.put(tag, uri);
    }
  }
  
  
  public synchronized String pretty(Element elem)
  {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    XMLWriter writer = null;
    
    try
    {
        writer = new XMLWriter(baos, OutputFormat.createPrettyPrint());
        writer.write(elem);
    }
    catch(Exception e) 
    {
        return elem.asXML();
    }
    
    return baos.toString();
  }
  
  public synchronized String pretty(Document doc)
  {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    XMLWriter writer = null;
    
    try
    {
        writer = new XMLWriter(baos, OutputFormat.createPrettyPrint());
        writer.write(doc);
    }
    catch(Exception e) 
    {
        return doc.asXML();
    }
    
    return baos.toString();
  }
  
  public synchronized String pretty(Node node)
  {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    XMLWriter writer = null;
    
    try
    {
        writer = new XMLWriter(baos, OutputFormat.createPrettyPrint());
        writer.write(node);
    }
    catch(Exception e) 
    {
        return node.asXML();
    }
    
    return baos.toString();
  }
  
  public synchronized Element getRootElement(){
      return document.getRootElement();
  }
  
  public synchronized Document getDocument()
  {
    return this.document;
  }

  
  
  protected Document document = null;
  
}
