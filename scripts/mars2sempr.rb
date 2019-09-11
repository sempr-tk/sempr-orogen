require 'orocos'
require 'readline'
require 'transformer/runtime'

include Orocos
Orocos.initialize

#Orocos.run 'sempr::SEMPREnvironment' => 'sempr',
#           'sempr::SEMPRTestDummy' => 'dummy', :gdb => ['sempr'] do

#Orocos.run 'sempr::SEMPREnvironment' => 'sempr', :gdb => true do
Orocos.run 'sempr::SEMPREnvironment' => 'sempr' do
    sempr = Orocos.name_service.get 'sempr'
#    dummy = Orocos.name_service.get 'dummy'
    mars = Orocos.name_service.get 'rh5_mars_fake_object_recognition'

    mars.detectionArray.connect_to sempr.detectionArray

    
    sempr.camera_frame = "Camera_Right"
    sempr.map_frame = "world"
    Orocos.transformer.setup(sempr)

    sempr.rdf_file = "../resources/combined.owl"
    sempr.rules_file = "../resources/owl.rules"
    sempr.configure
    sempr.start

#    dummy.sempr_task_name = "sempr"
#    dummy.configure
#    dummy.start

    Readline::readline("Press ENTER to exit\n")
end
