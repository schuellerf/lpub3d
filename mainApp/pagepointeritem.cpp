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
 * This class implements the graphical pointers that extend from the page to
 * as visual indicators to the builder as to where what the referenced
 * item is associated with on the page.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include <QGraphicsScene>
#include <QGraphicsView>
#include "pagepointeritem.h"

PagePointerItem::PagePointerItem(Meta          *meta,
  Pointer       *_pointer,
  QGraphicsItem *parent,
  LGraphicsView *_view)

  : PointerItem(parent)
{
  pointer             = *_pointer;

  PointerData pointerData = pointer.pointerMeta.value();
  placement           = pointerData.placement;

  BorderData  border  = meta->LPub.page.border.valuePixels();
  borderColor         = border.color;
  borderThickness     = border.thickness;

  QPointF base         = view->pageBackgroundItem->mapFromScene(QCursor::pos());
  baseX               = base.x();
  baseY               = base.y();

  if (pointerData.segments == OneSegment) {
      points[Tip] = QPoint(baseX + 10,baseY + 10);
    } else {
      points[Tip] = QPointF(pointerData.x1, pointerData.y1);
    }

  points[MidBase] = QPointF(pointerData.x3,pointerData.y3);
  points[MidTip]  = QPointF(pointerData.x4,pointerData.y4);

  QColor qColor(borderColor);
  QPen pen(qColor);
  pen.setWidth(borderThickness);
  pen.setCapStyle(Qt::RoundCap);

  // shaft segments
  for (int i = 0; i < pointerData.segments; i++) {
      QLineF linef;
      shaft = new QGraphicsLineItem(linef,this);
      shaft->setPen(pen);
      shaft->setZValue(-5);
      shaft->setFlag(QGraphicsItem::ItemIsSelectable,false);
      shaft->setToolTip(QString("Arrow segment %1 - drag to move; right click to modify").arg(i+1));
      shaftSegments.append(shaft);
      addToGroup(shaft);
  }

  autoLocFromTip();

  QPolygonF poly;

  head = new QGraphicsPolygonItem(poly, this);
  head->setPen(qColor);
  head->setBrush(qColor);
  head->setFlag(QGraphicsItem::ItemIsSelectable,false);
  head->setToolTip("Arrow head - drag to move");
  addToGroup(head);

  for (int i = 0; i < NumGrabbers; i++) {
    grabbers[i] = NULL;
  }

  drawPointerPoly();
  setFlag(QGraphicsItem::ItemIsFocusable,true);
}

void PagePointerItem::defaultPointer(){
  ;
}

bool PagePointerItem::autoLocFromTip(){
  return true;
}

bool PagePointerItem::autoLocFromMidBase(){
  return true;
}

void PagePointerItem::calculatePointerMetaLoc(){
  ;
}

void PagePointerItem::calculatePointerMeta(){
  ;
}
