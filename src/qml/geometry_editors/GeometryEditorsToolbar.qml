import QtQuick 2.12
import QtQml.Models 2.12
import org.qgis 1.0
import org.qfield 1.0
import Theme 1.0
import ".."

/**
This contains several geometry editing tools
A tool must subclass VisibilityFadingRow
And contains following functions:
  * function init(featureModel, mapSettings, editorRubberbandModel)
  * function cancel()
The following signal:
  * signal finished()
It can also implement:
  * blocking (bool) which prevents from swichting tools
*/

VisibilityFadingRow {
  id: geometryEditorsToolbar

  // the feature which has its geometry being edited
  property FeatureModel featureModel
  property MapSettings mapSettings
  // an additional Rubberband model for the tools (when drawing lines in split or addRing tools)
  property RubberbandModel editorRubberbandModel

  spacing: 4 * dp

  GeometryEditorsModel {
    id: editors
  }
  Component.onCompleted: {
    editors.addEditor(qsTr("Vertex Tool"), "ray-vertex", "VertexEditorToolbar.qml")
    editors.addEditor(qsTr("Split Tool"), "content-cut", "SplitFeatureToolbar.qml", GeometryEditorsModelSingleton.Line | GeometryEditorsModelSingleton.Polygon)
    // editors.addEditor(qsTr("Fill Ring Tool"), "picture_in_picture", "FillRingToolBar.qml", GeometryEditorsModelSingleton.Polygon)
  }

  function init() {
    selectorRow.stateVisible = false
    var lastUsed = settings.value( "/QField/GeometryEditorLastUsed", 0 )
    var toolbarQml = editors.data(editors.index(lastUsed,0), GeometryEditorsModelSingleton.ToolbarRole)
    var iconPath = editors.data(editors.index(lastUsed,0), GeometryEditorsModelSingleton.IconPathRole)
    toolbarRow.load(toolbarQml, iconPath)
  }

  function cancelEditors() {
    if (toolbarRow.item)
      toolbarRow.item.cancel()
    featureModel.vertexModel.clear()
  }

  // returns true if handled
  function canvasClicked(point) {
    if ( toolbarRow.item )
      return toolbarRow.item.canvasClicked(point)
    else
      return false
  }

  VisibilityFadingRow {
    id: selectorRow
    stateVisible: false

    spacing: 4 * dp

    Repeater {
      model: editors
      delegate: Button {
        round: true
        bgcolor: Theme.mainColor
        iconSource: Theme.getThemeIcon(iconPath)
        visible: GeometryEditorsModelSingleton.supportsGeometry(featureModel.vertexModel.geometry, supportedGeometries)
        onClicked: {
          // close current tool if any
          if (toolbarRow.item)
            toolbarRow.item.cancel()
          selectorRow.stateVisible = false
          toolbarRow.load(toolbar, iconPath)
          settings.setValue( "/QField/GeometryEditorLastUsed", index )
          displayToast(name)
        }
      }
    }
  }

  Loader {
    id: toolbarRow

    width: item && item.stateVisible ? item.implicitWidth : 0

    function load(qmlSource, iconPath){
      source = qmlSource
      item.init(geometryEditorsToolbar.featureModel, geometryEditorsToolbar.mapSettings, geometryEditorsToolbar.editorRubberbandModel)
      toolbarRow.item.stateVisible = true
      activeToolButton.iconSource = Theme.getThemeIcon(iconPath)
    }
  }

  Connections {
      target: toolbarRow.item
      onFinished: featureModel.vertexModel.clear()
  }

  Button {
    id: activeToolButton
    round: true
    visible: !selectorRow.stateVisible && !( toolbarRow.item && toolbarRow.item.stateVisible && toolbarRow.item.blocking )
    bgcolor: Theme.mainColor
    onClicked: {
      toolbarRow.source = ''
      selectorRow.stateVisible = true
    }
  }

}
