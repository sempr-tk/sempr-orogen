#!/usr/bin/ruby
require 'vizkit'
require 'orocos'
require 'typelib'
require 'Qt'
require 'ruby-graphviz'
require 'tmpdir'

Orocos.initialize

class SEMPRExplain < Qt::Object

  slots 'explainTriple(QTreeWidgetItem*, int)'
  slots 'toggleLayout()'
  slots 'changeDepth()'
  slots 'changeZoom(double)'

  def initialize(parent = nil)
    super(parent)
    @queryResults = []
    @window = Vizkit.load("explain.ui")
    @sempr = nil
    @lastExplainedTriple = nil

    # add a svg widget to display explanations
    @svgWidget = Qt::SvgWidget.new
    @svgWidget.setFixedHeight(200)
    @svgWidget.setFixedWidth(200)

    #@window.verticalLayout.addWidget @svgWidget
    @window.scrollArea.setWidget @svgWidget

    # connect stuff
    connect(@window.tripleList, SIGNAL('itemDoubleClicked(QTreeWidgetItem*, int)'), self, SLOT('explainTriple(QTreeWidgetItem*, int)'))
    connect(@window.radioButton, SIGNAL('clicked(bool)'), self, SLOT('toggleLayout()'))
    connect(@window.radioButton_2, SIGNAL('clicked(bool)'), self, SLOT('toggleLayout()'))
    connect(@window.spinBoxExplainLimit, SIGNAL('valueChanged(int)'), self, SLOT('changeDepth()'))
    connect(@window.doubleSpinBoxZoom, SIGNAL('valueChanged(double)'), self, SLOT('changeZoom(double)'))

    # a filename for the temporary svg to render
    @imageName = Dir::Tmpname.create(['explainTriple_', '.svg']){}
  end

  def show
    @window.show
  end

  def reconnectSEMPR
    @sempr = Orocos.name_service.get 'sempr'
  end


  def updateTriples()
    results = @sempr.listTriples("*", "*", "*")
    @queryResults.clear
    for triple in results
      @queryResults.push triple
    end

    @window.tripleList.clear
    for triple in @queryResults
      entry = Qt::TreeWidgetItem.new
      entry.setText(0, triple.subject_)
      entry.setText(1, triple.predicate_)
      entry.setText(2, triple.object_)
      @window.tripleList.addTopLevelItem entry
    end
  end

  def explainTriple(tripleItem, num)
    t = Types::sempr_rock::Triple.new
    t.subject_ = tripleItem.text(0)
    t.predicate_ = tripleItem.text(1)
    t.object_ = tripleItem.text(2)

    @lastExplainedTriple = t
    self.updateLastExplanation

    @window.tabWidget.setCurrentIndex(1)
  end
  

  def toggleLayout
    self.updateLastExplanation
  end

  def changeDepth
    self.updateLastExplanation
  end

  def changeZoom(zoom)
    defSize = @svgWidget.renderer().defaultSize()
    @svgWidget.setFixedWidth(defSize.width()*zoom)
    @svgWidget.setFixedHeight(defSize.height()*zoom)
  end
  
  def updateLastExplanation
    if not @lastExplainedTriple
      puts "Nothing to update"
      return
    end
    
    depth = @window.spinBoxExplainLimit.value()
    vertical = @window.radioButton_2.isChecked()

    explanation = @sempr.explainTriple(@lastExplainedTriple, depth, vertical)

    begin
      svg = GraphViz.parse_string(explanation){}.output(:svg => String)
      @svgWidget.load(Qt::ByteArray.new svg)
    rescue
      puts "Error creating Graph. Please try again."
    end

    zoom = @window.doubleSpinBoxZoom.value()
    defSize = @svgWidget.renderer().defaultSize()
    @svgWidget.setFixedWidth(defSize.width()*zoom)
    @svgWidget.setFixedHeight(defSize.height()*zoom)
  end
end



# run the gui
Orocos.run do
  explainGui = SEMPRExplain.new
  explainGui.reconnectSEMPR
  explainGui.show
  explainGui.updateTriples

  Vizkit.exec
end

