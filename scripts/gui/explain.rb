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

  def initialize(parent = nil)
    super(parent)
    @queryResults = []
    @window = Vizkit.load("explain.ui")
    @sempr = nil

    # add a svg widget to display explanations
    @svgWidget = Qt::SvgWidget.new
    @svgWidget.setFixedHeight(200)
    @svgWidget.setFixedWidth(200)

    #@window.verticalLayout.addWidget @svgWidget
    @window.scrollArea.setWidget @svgWidget

    # connect stuff
    connect(@window.tripleList, SIGNAL('itemDoubleClicked(QTreeWidgetItem*, int)'), self, SLOT('explainTriple(QTreeWidgetItem*, int)'))

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

    explanation = @sempr.explainTriple(t, 2, false)

    puts "#{@imageName}"
    svg = GraphViz.parse_string(explanation){}.output(:svg => String)
    @svgWidget.load(Qt::ByteArray.new svg)
    defSize = @svgWidget.renderer().defaultSize()
    @svgWidget.setFixedWidth(defSize.width()*1.5)
    @svgWidget.setFixedHeight(defSize.height()*1.5)

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

