/****************************************************************************
**
** Copyright (C) 2016 Trevor SANDY. All rights reserved.
**
** This file may be used under the terms of the
** GNU General Public Liceense (GPL) version 3.0
** which accompanies this distribution, and is
** available at http://www.gnu.org/licenses/gpl.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/**************************************************************************** 
 *
 * This class implements the graphical pointers that extend from callouts to
 * assembly images as visual indicators to the builder as to where to 
 * add the completed submodel into partially assembeled final model.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef POINTERITEMH
#define POINTERITEMH

#include "pointer.h"
#include "metaitem.h"
#include "resize.h"

class QGraphicsPolygonItem;
class QGraphicsLineItem;
class QGraphicsItemGroup;
class Callout;

class CalloutPointerItem : public QGraphicsItemGroup, public MetaItem, public AbstractResize 
{
public:
  CalloutPointerItem(
    Callout             *co,
    Meta                *meta,
    Pointer             *pointer,
    QGraphicsItem       *parent,
    QGraphicsView       *view);

  virtual ~CalloutPointerItem();

public:
  enum SelectedPoint { Tip, Base, MidBase, MidTip, NumGrabbers };
  enum ShaftSegments { OneSegment = 1, TwoSegments, ThreeSegments};

private:
  QGraphicsView        *view;
  Callout              *callout;
  QString               borderColor;
  float                 borderThickness;
  Pointer               pointer;
  PlacementEnc          placement;
  bool                  positionChanged;
  QGraphicsLineItem    *shaft;
  QGraphicsPolygonItem *head;
  QList<QGraphicsLineItem *> shaftSegments;
  
  Grabber              *grabbers[NumGrabbers];
  QPointF               points[NumGrabbers];  // the max points on the inside polygon
  
  virtual void placeGrabbers();
  virtual void resize(QPointF);
  virtual void change();
  virtual QGraphicsItem *myParentItem()
  {
    return parentItem();
  }

  /*
   *   +--------------------------------------------++
   *   |                                             |
   *   | . +-------------------------------------+   |
   *   |   |                                     | . |
   *   |   |                                     |   |
   *
   *
   *  callout size defines the outside edge of the callout.
   *  When there is a border, the inside rectangle starts
   *  at +thickness,+thickness, and ends at size-thickness,
   *  size-tickness.
   *
   *  Using round end cap caps the ends of the lines that
   *  intersect the callout are at +- tickness/2.  I'm not
   *  sure the affect of thickness is even vs. odd.
   *
   *  Loc should be calculated on the inside rectangle?
   *  The triangles have to go to the edge of the inner
   *  rectangle to obscure the border.
   *
   */

public:

  void drawTip(QPoint delta);

  /* When we drag the pointer tip, we move the tip relative
   * to the CSI rect, so we must recalculate the offset
   * into the CSI */

  void updatePointer(QPoint &delta);

  /* When the user "Add Pointer", we need to give a default/
     reasonable pointer */

  void defaultPointer();

private:
  /* Drag the tip of the pointer, and calculate a good
   * location for the pointer to connect to the callout. */

  bool autoLocFromTip();

  /* Drag the MidBase point of the pointer, and calculate a good
   * location for the pointer to connect to the callout. */

  bool autoLocFromMidBase();

  /* Drag the MidTip point of the pointer, and calculate a good
   * location for the MidBase segment to follow. */

  bool autoMidBaseFromMidTip();

  /* When we drag the CSI or the pointer's callout, we
   * need recalculate the Location portion of the pointer
   * meta, but the offset remains unchanged.
   * When we have more than one segment we calculate
   * from the Tip to the segment point and from the
   * Tip to the base when we have one segment (default) */

  void calculatePointerMetaLoc();

  void calculatePointerMeta();

  /* When using menu to add a new pointer, we need to add
   * a new line to the LDraw file. */

  void addPointerMeta();

  /* When we drag the callout or CSI, we need to recalculate
   * the pointer.
   * When we have more than one segment we calculate
   * from the CSI to the segment point and from the
   * CSI to the base when we have one segment (default) */

  void drawPointerPoly();
  bool autoLocFromBase(QPoint loc);

  /* Add shaft segment and control point grabber */
  void addShaftSegment();

  /* Remove shaft segment and control point grabber */
  void removeShaftSegment();

  int segments(){return shaftSegments.size();}


protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
};

#endif
