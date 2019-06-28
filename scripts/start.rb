require 'orocos'
require 'readline'
require 'vizkit'

include Orocos
Orocos.initialize

Orocos.run 'sempr::SEMPREnvironment' => 'sempr' do
    sempr = Orocos.name_service.get 'sempr'

    sempr.rdf_file = "../resources/combined.owl"
    sempr.rules_file = "../resources/owl.rules"
    sempr.configure
    sempr.start

    Vizkit.display sempr
    sov = Vizkit.default_loader.SpatialObjectVisualization
    sov.mesh_folder = "/home/transfit/rock_meshes/"
    Vizkit.display sempr.objectUpdatesBatch, :widget => sov

    Vizkit.exec
end
