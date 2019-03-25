require 'orocos'
require 'readline'

include Orocos
Orocos.initialize

Orocos.run do
    sempr = Orocos.name_service.get 'sempr'
    sempr.republish
end
