require 'orocos'
require 'readline'

include Orocos
Orocos.initialize

Orocos.run 'sempr::SEMPREnvironment' => 'sempr',
           'sempr::SEMPRTestDummy' => 'dummy' do
    sempr = Orocos.name_service.get 'sempr'
    dummy = Orocos.name_service.get 'dummy'

    sempr.configure
    sempr.start

    # dummy.configure
    # dummy.start

    Readline::readline("Press ENTER to exit\n")
end
