package org.proj4;

/**
 * Descriptor for a grid and its tree of sub-grids (if it has such).
 * Analog to the native PJ_GRIDINFO and CTABLE structures.
 * 
 * @author Svetlozar Kostadinov <sevarbg@gmail.com>
 */
public class GridInfo {
	
	/*
   * IMPLEMENTATION NOTE: Do not rename those fields, unless you update the
   * native C code accordingly.
   */
	
	public static enum Format 
	{
    CTABLE,
    CTABLE2,
    NTV1,
    NTV2, 
    GTX
	}
	
	public static class CTable
	{
		public String id;
		
		// The grid lower left cooridates in radians
		public double originLambda;
		public double originPhi;
		
		// The grid cell dimensions in radians
		public double cellLambda;
		public double cellPhi;
		
		// Number of grid columns and rows
		public int sizeLambda;
		public int sizePhi;
	}
	
	// The following comments are taken from PJ_GRIDINFO
	public String gridName;	/* identifying name of grid, eg "conus" or ntv2_0.gsb */
	public String fileName;	/* full path to filename */
	public Format format;		/* format of this grid, ie "ctable", "ntv1", "ntv2" or "missing". */
	public int gridOffset;	/* offset in file, for delayed loading */
	public CTable ctable;
	
	public GridInfo next;
	public GridInfo child;
}
