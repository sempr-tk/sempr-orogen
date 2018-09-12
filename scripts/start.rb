require 'orocos'
require 'readline'

include Orocos
Orocos.initialize

Orocos.run 'sempr::SEMPREnvironment' => 'sempr' do
    sempr = Orocos.name_service.get 'sempr'

    sempr.rdf_file = "../resources/model.owl"
    sempr.rules_file = "../resources/owl.rules"
    sempr.configure
    sempr.start

    Readline::readline("Press ENTER to exit\n")
end
