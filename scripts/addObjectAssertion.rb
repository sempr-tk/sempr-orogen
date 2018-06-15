require 'orocos'
require 'readline'
# require 'sempr-rock/typelib/ObjectAssertion'

include Orocos
Orocos.initialize

Orocos.run do
    sempr = Orocos.name_service.get 'sempr'


    while buf = Readline.readline("(objectId key value [baseURI])> ", true)
        parts = buf.split
        assertion = Types::sempr_rock::ObjectAssertion.new
        assertion.objectId = parts[0]
        assertion.key = parts[1]
        assertion.value = parts[2]
        if parts.length > 3
            assertion.baseURI = parts[3]
        end

        p sempr.addObjectAssertion assertion
    end
end
