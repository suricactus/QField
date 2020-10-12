/***************************************************************************
  rubberbandmodel.cpp - RubberbandModel

 ---------------------
 begin                : 10.6.2016
 copyright            : (C) 2016 by Matthias Kuhn
 email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "rubberbandmodel.h"
#include "snappingutils.h"

#include <qgsvectorlayer.h>
#include <qgsproject.h>
#include <qgslogger.h>

RubberbandModel::RubberbandModel( QObject *parent )
  : QObject( parent )
  , mCurrentCoordinateIndex( 0 )
  , mGeometryType( QgsWkbTypes::LineGeometry )
  , mLayer( nullptr )
{
  mPointList.insert( 0, QgsPoint() );
}

int RubberbandModel::vertexCount() const
{
  return mPointList.size();
}

bool RubberbandModel::isEmpty() const
{
  return mPointList.isEmpty();
}

QVector<QgsPoint> RubberbandModel::vertices() const
{
  return mPointList;
}

QVector<QgsPoint> RubberbandModel::flatVertices( bool skipCurrentPoint ) const
{
  QVector<QgsPoint> points;
  for ( const QgsPoint &pt : mPointList )
  {
    points << QgsPoint( pt );
  }
  if ( skipCurrentPoint )
    points.remove( mCurrentCoordinateIndex );

  return points;
}

QgsPointSequence RubberbandModel::pointSequence( const QgsCoordinateReferenceSystem &crs, QgsWkbTypes::Type wkbType, bool closeLine ) const
{
  QgsPointSequence sequence;
  QgsCoordinateTransform ct( mCrs, crs, QgsProject::instance()->transformContext() );

  for ( const QgsPoint &pt : mPointList )
  {
    //crs transformation of XY
    QgsPointXY p1 = ct.transform( pt.x(), pt.y() );

    //get point containing ZM if existing
    QgsPoint p2 = SnappingUtils::newPoint( pt, wkbType );
    p2.setX( p1.x() );
    p2.setY( p1.y() );

    //overwrite z and m values if already existent in the point
    if ( QgsWkbTypes::hasM( pt.wkbType() ) && QgsWkbTypes::hasM( wkbType ) )
    {
      p2.addMValue( pt.m() );
    }

    if ( QgsWkbTypes::hasZ( pt.wkbType() ) && QgsWkbTypes::hasZ( wkbType ) )
    {
      p2.addMValue( pt.z() );
    }

    sequence.append( p2 );
  }

  if ( closeLine && sequence.count() > 1 )
  {
    sequence.append( sequence.at( 0 ) );
  }
  return sequence;
}

QVector<QgsPointXY> RubberbandModel::flatPointSequence( const QgsCoordinateReferenceSystem &crs ) const
{
  QVector<QgsPointXY> sequence;

  QgsCoordinateTransform ct( mCrs, crs, QgsProject::instance()->transformContext() );

  for ( const QgsPoint &pt : mPointList )
  {
    sequence.append( ct.transform( pt.x(), pt.y() ) );
  }

  return sequence;
}

void RubberbandModel::setVertex( int index, QgsPoint coordinate )
{
  if ( mPointList.at( index ) != coordinate )
  {
    mPointList.replace( index, coordinate );
    emit vertexChanged( index );
  }
}

void RubberbandModel::insertVertices( int index, int count )
{
  for ( int i = 0; i < count; ++i )
  {
    mPointList.insert( index, currentCoordinate() );
  }

  emit verticesInserted( index, count );
  emit vertexCountChanged();
}

void RubberbandModel::removeVertices( int index, int count )
{
  if ( mPointList.size() <= 1 )
    return;

  mPointList.remove( index, count );
  emit verticesRemoved( index, count );
  emit vertexCountChanged();

  if ( mCurrentCoordinateIndex >= mPointList.size() )
  {
    setCurrentCoordinateIndex( mPointList.size() - 1 );
    emit currentCoordinateChanged();
  }
}

int RubberbandModel::currentCoordinateIndex() const
{
  return mCurrentCoordinateIndex;
}

void RubberbandModel::setCurrentCoordinateIndex( int currentCoordinateIndex )
{
  if ( currentCoordinateIndex == mCurrentCoordinateIndex )
    return;

  mCurrentCoordinateIndex = currentCoordinateIndex;
  emit currentCoordinateIndexChanged();
  emit currentCoordinateChanged();
}

QgsPoint RubberbandModel::currentPoint( const QgsCoordinateReferenceSystem &crs, QgsWkbTypes::Type wkbType ) const
{
  QgsCoordinateTransform ct( mCrs, crs, QgsProject::instance()->transformContext() );

  QgsPoint currentPt = mPointList.at( mCurrentCoordinateIndex );
  double x = currentPt.x();
  double y = currentPt.y();
  double z = QgsWkbTypes::hasZ( currentPt.wkbType() ) ? currentPt.z() : 0;
  double m = QgsWkbTypes::hasM( currentPt.wkbType() ) ? currentPt.m() : 0;

  try
  {
    ct.transformInPlace( x, y, z );
  }
  catch ( const QgsCsException &exp )
  {
    QgsDebugMsg( exp.what() );
  }
  catch(...)
  {
    // catch any other errors
    QgsDebugMsg( "Transform exception caught - possibly because of missing gsb file." );
  }

  QgsPoint resultPt( x, y );
  if ( QgsWkbTypes::hasZ( currentPt.wkbType() ) && QgsWkbTypes::hasZ( wkbType ) )
    resultPt.addZValue( z );
  if ( QgsWkbTypes::hasM( currentPt.wkbType() ) && QgsWkbTypes::hasM( wkbType ) )
    resultPt.addMValue( m );

  return resultPt;
}

QgsPoint RubberbandModel::currentCoordinate() const
{
  return mPointList.at( mCurrentCoordinateIndex );
}

void RubberbandModel::setCurrentCoordinate( const QgsPoint &currentCoordinate )
{
  // play safe, but try to find out
  // Q_ASSERT( mPointList.count() != 0 );
  if ( mPointList.count() == 0 )
    return;

  if ( mPointList.at( mCurrentCoordinateIndex ) == currentCoordinate )
    return;

  if ( mFrozen )
    return;

  mPointList.replace( mCurrentCoordinateIndex, currentCoordinate );
  emit currentCoordinateChanged();
  emit vertexChanged( mCurrentCoordinateIndex );
}

QDateTime RubberbandModel::currentPositionTimestamp() const
{
  return mCurrentPositionTimestamp;
}

void RubberbandModel::setCurrentPositionTimestamp( const QDateTime &currentPositionTimestamp )
{
  mCurrentPositionTimestamp = currentPositionTimestamp;
}

double RubberbandModel::measureValue() const
{
  return QgsWkbTypes::hasM( mPointList.at( mCurrentCoordinateIndex ).wkbType() ) ? mPointList.at( mCurrentCoordinateIndex ).m() : 0;
}

void RubberbandModel::setMeasureValue( const double measureValue )
{
  if ( mLayer && QgsWkbTypes::hasM( mLayer->wkbType() ) )
  {
    if ( !std::isnan( measureValue ) )
    {
      double mValue = measureValue;

      QgsPoint currentPoint = currentCoordinate();
      currentPoint.addMValue( mValue );
      setCurrentCoordinate( currentPoint );
    }
  }
}

void RubberbandModel::addVertex()
{
  // Avoid double vertices accidentally
  if ( mPointList.size() > 1 && *( mPointList.end() - 1 ) == *( mPointList.end() - 2 ) )
    return;

  insertVertices( mCurrentCoordinateIndex + 1, 1 );
  setCurrentCoordinateIndex( mCurrentCoordinateIndex + 1 );
}

void RubberbandModel::addVertexFromPoint( const QgsPoint &point )
{
  setCurrentCoordinate( point );
  addVertex();
}

void RubberbandModel::removeVertex()
{
  setCurrentCoordinateIndex( mCurrentCoordinateIndex - 1 );
  removeVertices( mCurrentCoordinateIndex + 1, 1 );
}

void RubberbandModel::reset()
{
  removeVertices( 0, mPointList.size() - 1 );
  mFrozen = false;
  emit frozenChanged();
}

void RubberbandModel::setDataFromGeometry( QgsGeometry geometry, const QgsCoordinateReferenceSystem &crs )
{
  if ( geometry.type() != mGeometryType )
    return;

  QgsCoordinateTransform ct( crs, mCrs, QgsProject::instance()->transformContext() );
  geometry.transform( ct );

  mPointList.clear();
  const QgsAbstractGeometry *abstractGeom = geometry.constGet();
  if ( !abstractGeom )
    return;

  QgsVertexId vertexId;
  QgsPoint pt;
  while ( abstractGeom->nextVertex( vertexId, pt ) )
  {
    if ( vertexId.part > 1 || vertexId.ring > 0 )
      break;

    // skip first vertex on polygon, as it's duplicate of the last one
    if ( geometry.type() == QgsWkbTypes::PolygonGeometry && vertexId.vertex == 0 )
      continue;

    mPointList << pt;
  }

  mCurrentCoordinateIndex = mPointList.size() - 1;

  emit verticesInserted( 0, mPointList.size() );
  emit vertexCountChanged();
}

QgsWkbTypes::GeometryType RubberbandModel::geometryType() const
{
  return mGeometryType;
}

void RubberbandModel::setGeometryType( const QgsWkbTypes::GeometryType &geometryType )
{
  if ( mGeometryType == geometryType )
    return;

  mGeometryType = geometryType;
  emit geometryTypeChanged();
}

QgsCoordinateReferenceSystem RubberbandModel::crs() const
{
  return mCrs;
}

void RubberbandModel::setCrs( const QgsCoordinateReferenceSystem &crs )
{
  if ( crs == mCrs )
    return;

  mCrs = crs;
  emit crsChanged();
}

QgsVectorLayer *RubberbandModel::vectorLayer() const
{
  return mLayer;
}

void RubberbandModel::setVectorLayer( QgsVectorLayer *vectorLayer )
{
  if ( vectorLayer == mLayer )
    return;

  mLayer = vectorLayer;

  if ( mLayer )
    setGeometryType( mLayer->geometryType() );

  emit vectorLayerChanged();
}

bool RubberbandModel::frozen() const
{
  return mFrozen;
}

void RubberbandModel::setFrozen( const bool &frozen )
{
  if ( mFrozen == frozen )
    return;

  mFrozen = frozen;

  emit frozenChanged();
}
