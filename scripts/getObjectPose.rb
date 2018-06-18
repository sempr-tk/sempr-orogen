require 'orocos'
require 'readline'

include Orocos
Orocos.initialize

Orocos.run do
    sempr = Orocos.name_service.get 'sempr'


    while buf = Readline.readline("> ", true)
        p sempr.getObjectPose(buf)
    end
end
