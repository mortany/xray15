#include "blobMesh.h"

UniformGrid::UniformGrid()
{
}

UniformGrid::~UniformGrid()
{
	FreeGrid();
}

void
UniformGrid::InitializeGrid(int size)
{
	width = size;
	if (width < 1) width = 1;
	widthXwidth = width * width;
	xGrid.SetCount(widthXwidth);
	yGrid.SetCount(widthXwidth);
	zGrid.SetCount(widthXwidth);

	for (int i = 0; i < widthXwidth; i++)
	{
		xGrid[i] = NULL;
		yGrid[i] = NULL;
		zGrid[i] = NULL;
		
	}
}

void
UniformGrid::FreeGrid()
{
	for (int j = 0; j < xGrid.Count(); j++)
	{
		if (xGrid[j])				
			delete xGrid[j];
	}

	for (int j = 0; j < yGrid.Count(); j++)
	{
		if (yGrid[j])				
			delete yGrid[j];
	}


	for (int j = 0; j < zGrid.Count(); j++)
	{
		if (zGrid[j])				
			delete zGrid[j];
	}



	xGrid.ZeroCount();
	yGrid.ZeroCount();
	zGrid.ZeroCount();

	xHitList.SetSize(0);
	yHitList.SetSize(0);
	zHitList.SetSize(0);
	hitList.ZeroCount();
}

int 
UniformGrid::FindCell(float x, float y, int whichGrid, int &ix, int &iy)
{

	float fCellX = 0.0f;
	float fCellY = 0.0f;
	float xWidth = 0.0f;
	float yWidth = 0.0f;

	if (whichGrid == 0)
	{
		fCellX = x - bounds.pmin.y;
		fCellY = y - bounds.pmin.z;
		xWidth = fYWidth;
		yWidth = fZWidth;
	}
	else if (whichGrid == 1)
	{
		fCellX = x - bounds.pmin.x;
		fCellY = y - bounds.pmin.z;
		xWidth = fXWidth;
		yWidth = fZWidth;
	}
	else if (whichGrid == 2)
	{
		fCellX = x - bounds.pmin.x;
		fCellY = y - bounds.pmin.y;
		xWidth = fXWidth;
		yWidth = fYWidth;
	}

	float xPer = 0.0f;
	float yPer = 0.0f;

	xPer = fCellX/xWidth;
	yPer = fCellY/yWidth;

	int iX = width * xPer;
	int iY = width * yPer;

	if (iX >= width) iX = width-1;
	if (iY >= width) iY = width-1;

	if (iX < 0) iX = 0;
	if (iY < 0) iY = 0;

	ix = iX;
	iy = iY;

	return iY * width + iX;

}

void
UniformGrid::LoadPoints(Point3 *p, float *radius, int count)
{
	int dx,dy;
	bounds.Init();
	pointBase.SetCount(count);
	radiusBase.SetCount(count);

	largestRadius = 0.0f;

	for (int i = 0; i < count; i++)
	{
		radiusBase[i] = radius[i];
		if (radius[i] > largestRadius) largestRadius = radius[i];
		pointBase[i] = p[i];
		bounds += p[i];
	}

	if (count == 1)
		bounds.EnlargeBy(0.5);


//need to do some bounds check to check for bounds that are to small
	float expandBy = 0.0f;

	if (((bounds.pmax.x - bounds.pmin.x)/width) < 0.0001f)
		expandBy += 0.1f * width;
	if (((bounds.pmax.y - bounds.pmin.y)/width) < 0.0001f)
		expandBy += 0.1f * width;
	if (((bounds.pmax.z - bounds.pmin.z)/width) < 0.0001f)
		expandBy += 0.1f * width;

	bounds.EnlargeBy(expandBy);


	fXWidth = bounds.pmax.x - bounds.pmin.x;
	fYWidth = bounds.pmax.y - bounds.pmin.y;
	fZWidth = bounds.pmax.z - bounds.pmin.z;

	for (int i = 0; i < count; i++)
	{
		
	//do XGrid
	//find out which cell we are in
		int index;
		index = FindCell(p[i].y,p[i].z,0,dx,dy);

		if ((index < 0) || (index >= widthXwidth))
			DebugPrint(_T("XGrid Error cell out of range %d\n"),i);
		else
			{
		//see if cell exists
			if (xGrid[index] == NULL)
				xGrid[index] = new Grid();
		//add that vertex to the cell
			xGrid[index]->index.Append(1,&i,10);
			xGrid[index]->hit=FALSE;
			}

	//do yGrid
	//find out which cell we are in
		index = FindCell(p[i].x,p[i].z,1,dx,dy);

		if ((index < 0) || (index >= widthXwidth))
			DebugPrint(_T("YGrid Error cell out of range %d\n"),i);
		else
			{
		//see if cell exists
			if (yGrid[index] == NULL)
				yGrid[index] = new Grid();
		//add that vertex to the cell
			yGrid[index]->index.Append(1,&i,10);			
			yGrid[index]->hit=FALSE;
			}


	//do ZGrid
	//find out which cell we are in
		index = FindCell(p[i].x,p[i].y,2,dx,dy);

		if ((index < 0) || (index >= widthXwidth))
			DebugPrint(_T("ZGrid Error cell out of range %d\n"),i);
		else
			{
		//see if cell exists
			if (zGrid[index] == NULL)
				zGrid[index] = new Grid();
		//add that vertex to the cell
			zGrid[index]->index.Append(1,&i,10);			
			zGrid[index]->hit=FALSE;
			}


	}

	xHitList.SetSize(count);
	yHitList.SetSize(count);
	zHitList.SetSize(count);
}

void
UniformGrid::TagCells(Point3 p,float radius, int whichGrid)
{
	float x = 0.0f, y = 0.0f;
	int minX,maxX;
	int minY,maxY;

//XGrid
	if (whichGrid == 0)
		{
		x = p.y - radius;
		y = p.z - radius;
		}
//YGrid
	else if (whichGrid == 1)
		{
		x = p.x - radius;
		y = p.z - radius;
		}
//YGrid
	else if (whichGrid == 2)
		{
		x = p.x - radius;
		y = p.y - radius;
		}


	FindCell(x, y, whichGrid, minX, minY);

//XGrid
	if (whichGrid == 0)
		{
		x = p.y + radius;
		y = p.z + radius;
		}
//YGrid
	else if (whichGrid == 1)
		{
		x = p.x + radius;
		y = p.z + radius;
		}
//YGrid
	else if (whichGrid == 2)
		{
		x = p.x + radius;
		y = p.y + radius;
		}

	FindCell(x, y, whichGrid, maxX, maxY);

	if (minX < 0) minX = 0;
	if (minY < 0) minY = 0;

	if (maxX >= width) maxX = width-1;
	if (maxY >= width) maxY = width-1;
//fill out the bitarray
	for (int i = minY; i <= maxY; i++)
	{
		int index = i * width+minX;
		for (int j = minX; j <= maxX; j++)
		{
			Grid *g = NULL;

			if (whichGrid == 0)
				{
				g = xGrid[index];
				}
			else if (whichGrid == 1)
				{
				g = yGrid[index];
				}
			else if (whichGrid == 2)
				{
				g = zGrid[index];
				}

			if (g)
			{

				for (int k = 0; k < g->index.Count(); k++)
				{
					int pointIndex = g->index[k];
					hitList.Append(1,&pointIndex,1000);
					if (whichGrid == 0)
						xHitList.Set(pointIndex);
					else if (whichGrid == 1)
						yHitList.Set(pointIndex);
					else if (whichGrid == 2)
						zHitList.Set(pointIndex);
				}
			}
			index++;
		}
	}


}

void
UniformGrid::ClosestPoint(Point3 p, float radius, int &pindex, float &d)
{
	xHitList.ClearAll();
	yHitList.ClearAll();
	zHitList.ClearAll();
	hitList.SetCount(0);

	//find the cell in the XGrid
	TagCells(p,radius, 0);
	//find the cell in the YGrid
	TagCells(p,radius, 1);
	//find the cell in the ZGrid
	TagCells(p,radius, 2);

	BitArray usedList;
	usedList.SetSize(pointBase.Count());
	usedList.ClearAll();

	int closest = -1;
	d = 0.0f;
	Box3 localBounds;
	localBounds.Init();
	localBounds += p;
	localBounds.EnlargeBy(radius);


	for (int i = 0; i < hitList.Count(); i++)
	{
		int index = hitList[i];
		if (!usedList[index])  //check to see if we have processed this one or not
		{
			if (xHitList[index] && yHitList[index] && zHitList[index])
			{
				usedList.Set(index);
				Point3 source = pointBase[index];
				if (localBounds.Contains(source))
				{
					float dist = LengthSquared(source-p);
					if ((dist < d) || (closest == -1))
					{
						d = dist;
						closest = index;
					}
				}
			}
		}
	}
	pindex = closest;
	d = sqrt(d);


}

void
UniformGrid::ClosestPoints(Point3 *p, float *radius, int *i, float *d, int count)
{
}

void
UniformGrid::InRadius(Point3 p,  Tab<int> &indexList)
{
	float radius = largestRadius;
	xHitList.ClearAll();
	yHitList.ClearAll();
	zHitList.ClearAll();
	hitList.SetCount(0);

	//find the cell in the XGrid
	TagCells(p,radius, 0);
	//find the cell in the YGrid
	TagCells(p,radius, 1);
	//find the cell in the ZGrid
	TagCells(p,radius, 2);

	BitArray usedList;
	usedList.SetSize(pointBase.Count());
	usedList.ClearAll();

	int closest = -1;
	float d = 0.0f;
	Box3 localBounds;
	localBounds.Init();
	localBounds += p;
	localBounds.EnlargeBy(radius);


	for (int i = 0; i < hitList.Count(); i++)
	{
		int index = hitList[i];
		if (!usedList[index])  //check to see if we have processed this one or not
		{
			if (xHitList[index] && yHitList[index] && zHitList[index])
			{
				usedList.Set(index);
				Point3 source = pointBase[index];
				if (localBounds.Contains(source))
				{
					indexList.Append(1,&index,1000);

				}
			}
		}
	}

}

void
UniformGrid::DrawBounds(GraphicsWindow *gw, Box3 bounds)
{

	Point3 plist[3];
	
	plist[0] = bounds[0];
	plist[1] = bounds[1];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);

	plist[0] = bounds[1];
	plist[1] = bounds[3];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);

	plist[0] = bounds[3];
	plist[1] = bounds[2];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);

	plist[0] = bounds[2];
	plist[1] = bounds[0];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);


	plist[0] = bounds[0+4];
	plist[1] = bounds[1+4];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);

	plist[0] = bounds[1+4];
	plist[1] = bounds[3+4];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);

	plist[0] = bounds[3+4];
	plist[1] = bounds[2+4];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);

	plist[0] = bounds[2+4];
	plist[1] = bounds[0+4];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);
					

	plist[0] = bounds[0];
	plist[1] = bounds[0+4];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);

	plist[0] = bounds[1];
	plist[1] = bounds[1+4];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);

	plist[0] = bounds[3];
	plist[1] = bounds[3+4];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);

	plist[0] = bounds[2];
	plist[1] = bounds[2+4];
	gw->polyline(2, plist, NULL, NULL, 1,NULL);
}

void
UniformGrid::Display(GraphicsWindow *gw)
{
	return;
	Matrix3 tm(1);
	gw->setTransform(tm);
	DrawBounds(gw, bounds);

	float sx,sy,sz;
	float incx,incy,incz;

	sx = 0.0f;
	sy = bounds.pmin.y;
	sz = bounds.pmin.z;
	incx = 0.0f;
	incy = (bounds.pmax.y - bounds.pmin.y)/width;
	incz = (bounds.pmax.z - bounds.pmin.z)/width;


	int index =0;
	float x,y,z;
	z = sz;
	for (int i = 0; i < width; i++)
	{
		x = 0.0;
		y = sy;
		for (int j = 0; j < width; j++)
		{
			Box3 b;
			b.Init();
			Point3 p(x,y,z);
			b += p;
			p.x += incx;
			p.y += incy;
			p.z += incz; 
			b += p;

			if (xGrid[index]->hit)
				{
				DrawBounds(gw, b);
				gw->marker(&b.Center(),ASTERISK_MRKR);

				}
			else DrawBounds(gw, b);
			index++;
			y += incy;
		}
		z += incz;
	}

	sx = bounds.pmin.x;
	sy = 0.0f;
	sz = bounds.pmin.z;
	incx = (bounds.pmax.x - bounds.pmin.x)/width;
	incy = 0.0f;
	incz = (bounds.pmax.z - bounds.pmin.z)/width;

/*
	index =0;
	z = sz;
	for (i = 0; i < width; i++)
	{
		x = sx;
		y = 0.0;
		for (int j = 0; j < width; j++)
		{
			Box3 b;
			b.Init();
			Point3 p(x,y,z);
			b += p;
			p.x += incx;
			p.y += incy;
			p.z += incz; 
			b += p;

			if (yGrid[index]->hit)
				{
				DrawBounds(gw, b);
				gw->marker(&b.Center(),CIRCLE_MRKR);

				}
			else DrawBounds(gw, b);
			index++;
			x += incx;
		}
		z += incz;
	}
*/
	gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
	gw->marker(&debugP,BIG_BOX_MRKR);


	Point3 red (1.0f,0.0f,0.0f);

	gw->setColor(LINE_COLOR,red);
	gw->marker(&bounds.pmin,BIG_BOX_MRKR);
}