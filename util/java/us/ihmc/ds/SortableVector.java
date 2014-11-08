/**
 * SortableVector.java
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
 * Implementation of a vector that can be sorted using quicksort
 * 
 * By IHMC - UWF
 *	Raul Saavedra, March/02/2000
 *	rsaavedr@ai.uwf.edu, saavedra@eecs.tulane.edu
 */

package us.ihmc.ds;

import java.util.Enumeration;
import java.util.Vector;

public class SortableVector extends Vector
{
	public SortableVector ()
	{
		super ();
	}
	
	public SortableVector (int capac)
	{
		super (capac);
	}

	public SortableVector (Vector v)
	{
		super (v.size()+1);
		int i = 0;
		int j = v.size();
		while (i<j) {
			this.addElement(v.elementAt(i));
			i++;
		}
	}

	public SortableVector (Enumeration E)
	{
		super ();
		while (E.hasMoreElements()) {
			this.addElement(E.nextElement());
		}
	}
	
	public SortableVector (Enumeration E, int nelems)
	{
		super (Math.max (17, nelems+1));
		while (E.hasMoreElements()) {
			this.addElement(E.nextElement());
		}
	}
	
	// Sorts the vector using quick-sort
	public void sort()
	{
		int p = 0;
		int r = this.size()-1;
		quickSort (p, r);
	}

	private void quickSort (int p, int r)
	{
		if (p < r) {
			int q = quickSortPart (p, r);
			quickSort (p, q);
			quickSort (q+1, r);
		}
	}
	
	private int quickSortPart (int p, int r)
	{
		String Vp = this.elementAt(p).toString();
		int i = p - 1;
		int j = r + 1;
		while (true) {
			do j--; while ((this.elementAt(j).toString()).compareTo(Vp) > 0);
			do i++; while ((this.elementAt(i).toString()).compareTo(Vp) < 0);
			if (i < j) {
				Object Vi = this.elementAt(i);
				this.setElementAt(this.elementAt(j), i);
				this.setElementAt(Vi, j);
			}
			else return j;
		}
	}
}
