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

#include <QPen>
#include <QGraphicsSceneMouseEvent>
#include "lgraphicsscene.h"

LGraphicsScene::LGraphicsScene(QObject *parent)
  : QGraphicsScene(parent)
{
  guidePen      = QPen(Qt::black, 1);
  guidesEnabled = true;
}

void LGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  //TODO - consider conditional if better performance.
  mMousePos = event->scenePos();
  invalidate(this->sceneRect(),QGraphicsScene::ForegroundLayer);
}

void LGraphicsScene::drawForeground(QPainter *painter, const QRectF &rect){
  if (! guidesEnabled)
    return;

  painter->setClipRect(rect);
  painter->setPen(guidePen);
  painter->drawLine(mMousePos.x(), rect.top(), mMousePos.x(), rect.bottom());
  painter->drawLine(rect.left(), mMousePos.y(), rect.right(), mMousePos.y());

}
