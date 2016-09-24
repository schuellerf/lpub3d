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

#include "pagepointeritem.h"
#include "pagebackgrounditem.h"
#include "step.h"
#include "range.h"
#include "ranges.h"

//---------------------------------------------------------------------------

/*
 * This is the constructor of a graphical pointer
 */

PagePointerItem::PagePointerItem(
  Page              *_pg,
  PointerAttributes *_attributes,
  Pointer           *_pointer,
  QGraphicsItem     *parent,
  LGraphicsView     *_view)

  : PointerItem(parent)
{
  page          = _pg;
  view          = _view;
  pa            = _attributes;
  pointer       = *_pointer;

  if (page->list.size()) {
      Range *range = dynamic_cast<Range *>(page->list[0]);
      if (range->relativeType == RangeType) {
          step = dynamic_cast<Step *>(range->list[0]);
          if (step && step->relativeType == StepType) {
              logDebug() << "Page is Step: ";
            }
        }
    }

  PointerData pointerData = pointer.pointerMeta.value();
  BorderData  border = page->meta.LPub.page.border.valuePixels();
  PlacementData pointerBasePlacement = page->meta.LPub.page.pointerBase.value();

  borderColor     = border.color;
  borderThickness = border.thickness;

  placement       = pointerData.placement;

  baseX           = pa->size[XX];
  baseY           = pa->size[YY];

  if (pointerData.segments == OneSegment) {
      int cX = step->csiItem->loc[XX];
      int cY = step->csiItem->loc[YY];
      int dX = pointerData.x1*step->csiItem->size[XX];
      int dY = pointerData.y1*step->csiItem->size[YY];

      if (pa->placement.value().relativeTo == PageType) {
          cX += step->loc[XX];
          cY += step->loc[YY];
          points[Tip] = QPoint(cX + dX - pa->loc[XX], cY + dY - pa->loc[YY]);
      } else {
          points[Tip] = QPoint(cX + dX - pa->loc[XX], cY + dY - pa->loc[YY]);
      }
       /*
       * What does it take to convert csiItem->loc[] and size[] to the position of
       * the tip in these cases:
       *   single step
       *     callout relative to csi
       *     callout relative to stepNumber
       *     callout relative to pli
       *     callout relative to page
       *
       *   step group
       *     callout relative to csi
       *     callout relative to stepNumber
       *     callout relative to pli
       *     callout relative to page
       *     callout relative to stepGroup
       */
      if ( ! step->onlyChild()) {
          switch (pointerBasePlacement.relativeTo) {
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

  QColor qColor(border.color);
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

/*
 * Given the location of the Tip (as dragged around by the user)
 * calculate a reasonable placement and Loc for Base or MidTip points.
 */

bool PagePointerItem::autoLocFromTip()
{
    int width = pa->size[XX];
    int height = pa->size[YY];
    int left = 0;
    int right = width;
    int top = 0;
    int bottom = height;

    QPoint intersect;
    int tx,ty;

    tx = points[Tip].x();
    ty = points[Tip].y();

    if (segments() != OneSegment) {
        PointerData pointerData = pointer.pointerMeta.value();
        int mx = pointerData.x3;
        points[Base] = QPointF(pointerData.x2,pointerData.y2);
        placement = pointerData.placement;

        if (segments() == ThreeSegments){
            points[MidTip].setY(ty);
            points[MidTip].setX(mx);
        }
    } else {
        /* Figure out which corner */
        //BorderData borderData = callout->background->border.valuePixels();
        int radius = 2;// (int) borderData.radius;

        if (ty >= top+radius && ty <= bottom-radius) {
            if (tx < left) {
                intersect = QPoint(left,ty);
                points[Tip].setY(ty);
                placement = Left;
            } else if (tx > right) {
                intersect = QPoint(right,ty);
                points[Tip].setY(ty);
                placement = Right;
            } else {
                // inside
                placement = Center;
            }
        } else if (tx >= left+radius && tx <= right-radius) {
            if (ty < top) {
                intersect = QPoint(tx,top);
                points[Tip].setX(tx);
                placement = Top;
            } else if (ty > bottom) {
                intersect = QPoint(tx,bottom);
                points[Tip].setX(tx);
                placement = Bottom;
            } else {
                // inside
                placement = Center;
            }
        } else if (tx < radius) {  // left?
            if (ty < radius) {
                intersect = QPoint(left+radius,top+radius);
                placement = TopLeft;
            } else {
                intersect = QPoint(radius,height-radius);
                placement = BottomLeft;
            }
        } else { // right!
            if (ty < radius) {
                intersect = QPoint(width-radius,radius);
                placement = TopRight;
            } else {
                intersect = QPoint(width-radius,height-radius);
                placement = BottomRight;
            }
        }

        points[Base] = intersect;
    }

    return true;
}

/*
 * Given the location of the MidBase point (as dragged around by the user)
 * calculate a reasonable placement and Loc for Base point.
 */

bool PagePointerItem::autoLocFromMidBase()
{
        int width = pa->size[XX];
        int height = pa->size[YY];
        int left = 0;
        int right = width;
        int top = 0;
        int bottom = height;

        QPoint intersect;
        int tx,ty;

        tx = points[MidBase].x();
        ty = points[MidBase].y();

        /* Figure out which corner */
        //BorderData borderData = callout->background->border.valuePixels();
        int radius = 2;// (int) borderData.radius;

        if (ty >= top+radius && ty <= bottom-radius) {
            if (tx < left) {
                intersect = QPoint(left,ty);
                points[MidBase].setY(ty);
                placement = Left;
            } else if (tx > right) {
                intersect = QPoint(right,ty);
                points[MidBase].setY(ty);
                placement = Right;
            } else {
                // inside
                placement = Center;
            }
        } else if (tx >= left+radius && tx <= right-radius) {
            if (ty < top) {
                intersect = QPoint(tx,top);
                points[MidBase].setX(tx);
                placement = Top;
            } else if (ty > bottom) {
                intersect = QPoint(tx,bottom);
                points[MidBase].setX(tx);
                placement = Bottom;
            } else {
                // inside
                placement = Center;
            }
        } else if (tx < radius) {  // left?
            if (ty < radius) {
                intersect = QPoint(left+radius,top+radius);
                placement = TopLeft;
            } else {
                intersect = QPoint(radius,height-radius);
                placement = BottomLeft;
            }
        } else { // right!
            if (ty < radius) {
                intersect = QPoint(width-radius,radius);
                placement = TopRight;
            } else {
                intersect = QPoint(width-radius,height-radius);
                placement = BottomRight;
            }
        }

        points[MidTip].setX(tx);
        points[Base] = intersect;

    return true;
}

void PagePointerItem::defaultPointer()
{
  points[Tip] = QPointF(step->csiItem->loc[XX]+
                        step->csiItem->size[XX]/2,
                        step->csiItem->loc[YY]+
                        step->csiItem->size[YY]/2);

  if ( ! step->onlyChild()) {
    PlacementData pointerBasePlacement = pa->placement.value();
    switch (pointerBasePlacement.relativeTo) {
      case PageType:
      case StepGroupType:
        points[Tip] += QPoint(step->grandparent()->loc[XX],
                              step->grandparent()->loc[YY]);
        points[Tip] += QPoint(step->range()->loc[XX],
                              step->range()->loc[YY]);
        points[Tip] += QPoint(step->loc[XX],
                              step->loc[YY]);
        points[Tip] -= QPoint(pa->loc[XX],pa->loc[YY]);
      break;
      default:
      break;
    }
  }
  autoLocFromTip();
  drawPointerPoly();
  calculatePointerMeta();
  addPointerMeta();
}

/*
 * Given the location of the Tip (as dragged around by the user)
 * calculate a reasonable placement and Loc for Base or MidTip points.
 */

void PagePointerItem::drawPointerPoly()
{

  for (int i = 0; i < segments(); i++) {

      QLineF linef;
      switch (segments()) {
      case OneSegment:
      {
          linef = QLineF(points[Base],points[Tip]);
          removeFromGroup(shaftSegments[i]);
          QGraphicsLineItem    *shaft = shaftSegments[i];
          shaft->setLine(linef);
          addToGroup(shaft);
      }
          break;
      case TwoSegments:
      {
          if (i == 0) {
              linef = QLineF(points[Base],points[MidBase]);
              removeFromGroup(shaftSegments[i]);
              QGraphicsLineItem    *shaft = shaftSegments[i];
              shaft->setLine(linef);
              addToGroup(shaft);
          } else {
              linef = QLineF(points[MidBase],points[Tip]);
              removeFromGroup(shaftSegments[i]);
              QGraphicsLineItem    *shaft = shaftSegments[i];
              shaft->setLine(linef);
              addToGroup(shaft);
          }
      }
          break;
      case ThreeSegments:
      {
          if (i == 0) {
              linef = QLineF(points[Base],points[MidBase]);
              removeFromGroup(shaftSegments[i]);
              QGraphicsLineItem    *shaft = shaftSegments[i];
              shaft->setLine(linef);
              addToGroup(shaft);
          } else if (i == 1){
              linef = QLineF(points[MidBase],points[MidTip]);
              removeFromGroup(shaftSegments[i]);
              QGraphicsLineItem    *shaft = shaftSegments[i];
              shaft->setLine(linef);
              addToGroup(shaft);
          } else {
              linef = QLineF(points[MidTip],points[Tip]);
              removeFromGroup(shaftSegments[i]);
              QGraphicsLineItem    *shaft = shaftSegments[i];
              shaft->setLine(linef);
              addToGroup(shaft);
          }
      }
          break;
      default:
          break;
      }

      if (shaftSegments.last()){
          // head
          QPolygonF poly;

          poly << QPointF(-2*grabSize(), 0);
          poly << QPointF(-2*grabSize(),grabSize()/2);
          poly << QPointF(grabSize()/2,0);
          poly << QPointF(-2*grabSize(),-grabSize()/2);
          poly << QPointF(-2*grabSize(),0);

          removeFromGroup(head);
          head->setPolygon(poly);

          qreal x;
          qreal y;
          switch (segments()) {
          case OneSegment:
              x = points[Tip].x()-points[Base].x();
              y = points[Tip].y()-points[Base].y();
              break;
          case TwoSegments:
              x = points[Tip].x()-points[MidBase].x();
              y = points[Tip].y()-points[MidBase].y();
              break;
          case ThreeSegments:
              x = points[Tip].x()-points[MidTip].x();
              y = points[Tip].y()-points[MidTip].y();
              break;
          default:
              break;
          }

          qreal h = sqrt(x*x+y*y);
          qreal angle = 180*acos(x/h);

          qreal pi = 22.0/7;

          if (x == 0) {
            if (y < 0) {
              angle = 270.0;
            } else {
              angle = 90.0;
            }
          } else if (y == 0) {
            if (x < 0) {
              angle = 180.0;
            } else {
              angle = 0.0;
            }
          } else {
            volatile qreal h = sqrt(x*x+y*y);
            if (x > 0) {
              if (y > 0) {
                angle = 180-180*acos(-x/h)/pi;
              } else {
                angle = 180+180*acos(-x/h)/pi;
              }
            } else {
              if (y > 0) {
                angle = 180*acos(x/h)/pi;
              } else {
                angle = -180*acos(x/h)/pi;
              }
            }
          }

          head->resetTransform();
          head->setRotation(rotation() + angle);
          head->setPos(points[Tip]);
          addToGroup(head);
      }
  }

  view->updateSceneRect(sceneBoundingRect());
}

void PagePointerItem::calculatePointerMetaLoc()
{
  float loc = 0;

  switch (placement) {
    case TopLeft:
    case TopRight:
    case BottomLeft:
    case BottomRight:
      loc = 0;
    break;
    case Top:
    case Bottom:
    {
      if (segments() == OneSegment)
         loc = points[Base].x()/pa->size[XX];
      else
         loc = points[Base].x();
    }
    break;
    case Left:
    case Right:
    {
      if (segments() == OneSegment)
         loc = points[Base].y()/pa->size[YY];
      else
         loc = points[Base].y();
    }
    break;
    default:
    break;
  }
  PointerData pointerData = pointer.pointerMeta.value();
  pointer.pointerMeta.setValue(
    placement,
    loc,
    0,
    segments(),
    pointerData.x1,
    pointerData.y1,
    pointerData.x2,
    pointerData.y2,
    pointerData.x3,
    pointerData.y3,
    pointerData.x4,
    pointerData.y4);
}

void PagePointerItem::calculatePointerMeta()
{
  calculatePointerMetaLoc();

  PointerData pointerData = pointer.pointerMeta.value();
  if (segments() == OneSegment) {
      if (step->onlyChild()) {
          points[Tip] += QPoint(pa->loc[XX],pa->loc[YY]);
      } else {
          PlacementData pointerBasePlacement = page->meta.LPub.page.pointerBase.value();

          switch (pointerBasePlacement.relativeTo) {
          case CsiType:
          case PartsListType:
          case StepNumberType:
              points[Tip] += QPoint(pa->loc[XX],pa->loc[YY]);
              break;
          case PageType:
          case StepGroupType:
              points[Tip] -= QPoint(step->grandparent()->loc[XX],
                                    step->grandparent()->loc[YY]);
              points[Tip] -= QPoint(step->loc[XX],
                                    step->loc[YY]);
              points[Tip] += QPoint(pa->loc[XX],pa->loc[YY]);
              break;
          default:
              break;
          }
      }

      if (pa->placement.value().relativeTo == PageType) {
          points[Tip] -= QPoint(step->loc[XX],
                                step->loc[YY]);
      }

      pointerData.x1 = (points[Tip].x() - step->csiItem->loc[XX])/step->csiItem->size[XX];
      pointerData.y1 = (points[Tip].y() - step->csiItem->loc[YY])/step->csiItem->size[YY];
  } else {
      pointerData.x1 = points[Tip].x();
      pointerData.y1 = points[Tip].y();
      pointerData.x2 = points[Base].x();
      pointerData.y2 = points[Base].y();
      pointerData.x3 = points[MidBase].x();
      pointerData.y3 = points[MidBase].y();
      pointerData.x4 = points[MidTip].x();
      pointerData.y4 = points[MidTip].y();
  }

  pointer.pointerMeta.setValue(
    placement,
    pointerData.loc,
    0,
    segments(),
    pointerData.x1,
    pointerData.y1,
    pointerData.x2,
    pointerData.y2,
    pointerData.x3,
    pointerData.y3,
    pointerData.x4,
    pointerData.y4);
}
