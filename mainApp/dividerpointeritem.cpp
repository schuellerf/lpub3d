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
 * This class implements the graphical pointers that extend from the divider to
 * as visual indicators to the builder as to where what the referenced
 * item is associated with on the page.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "dividerpointeritem.h"
#include "step.h"
#include "range.h"
#include "ranges.h"


DividerPointerItem::DividerPointerItem(
  DividerItem   *divider,
  Meta          *meta,
  Pointer       *_pointer,
  QGraphicsItem *parent,
  QGraphicsView *_view)

  : PointerItem(parent)
{
  view            = _view;
  pointer         = *_pointer;
  step            = divider->step;
  steps           = step->grandparent();
  sepData         = step->range()->sepMeta.valuePixels();

  PointerData pointerData = pointer.pointerMeta.value();
  PlacementData dividerPlacement = meta->LPub.multiStep.placement.value();

  borderColor     = sepData.color;
  borderThickness = sepData.thickness;

  placement       = pointerData.placement;

  baseX           = divider->lineItem->x();
  baseY           = divider->lineItem->y();

  if (pointerData.segments == OneSegment) {
      int cX = step->csiItem->loc[XX];
      int cY = step->csiItem->loc[YY];
      int dX = pointerData.x1*step->csiItem->size[XX];
      int dY = pointerData.y1*step->csiItem->size[YY];

      if (steps->placement.value().relativeTo == CalloutType) {
          cX += step->loc[XX];
          cY += step->loc[YY];
          points[Tip] = QPoint(cX + dX - steps->loc[XX], cY + dY - steps->loc[YY]);
      } else {
          points[Tip] = QPoint(cX + dX - steps->loc[XX], cY + dY - steps->loc[YY]);
      }

       /*
       * What does it take to convert csiItem->loc[] and size[] to the position of
       * the tip in these cases:
       *   single step
       *   step group
       */
      if ( ! step->onlyChild()) {
          switch (dividerPlacement.relativeTo) {
          case PageType:
          case StepGroupType:
              points[Tip] += QPoint(step->grandparent()->loc[XX],
                                    step->grandparent()->loc[YY]);
              points[Tip] += QPoint(step->range()->loc[XX],
                                    step->range()->loc[YY]);
              points[Tip] += QPoint(step->loc[XX],
                                    step->loc[YY]);
              break;
          default:
              break;
          }
      }
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

void DividerPointerItem::defaultPointer(){
  ;
}

bool DividerPointerItem::autoLocFromTip(){
  return true;
}

bool DividerPointerItem::autoLocFromMidBase(){
  return true;
}

void DividerPointerItem::calculatePointerMetaLoc(){
  ;
}

void DividerPointerItem::calculatePointerMeta(){
  ;
}
