require 'orocos'
require 'readline'

include Orocos
Orocos.initialize

Orocos.run 'sempr::SEMPRTestDummy' => 'testdummy' do
    dummy = Orocos.name_service.get 'testdummy'

#    dummy.configure
#    dummy.start
    Readline::readline('Press [ENTER] to exit')
end
