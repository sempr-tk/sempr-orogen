require 'orocos'
require 'readline'

include Orocos
Orocos.initialize

Orocos.run 'sempr::SEMPREnvironment' => 'sempr' do
    sempr = Orocos.name_service.get 'sempr'

    sempr.configure
    sempr.start

    Readline::readline("Press ENTER to exit\n")
end
