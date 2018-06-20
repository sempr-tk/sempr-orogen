require 'orocos'
require 'readline'
# require 'sempr-rock/typelib/ObjectAssertion'

include Orocos
Orocos.initialize

Orocos.run do

    while buf = Readline.readline("(entity subject predicate value)> ", true)
        sempr = Orocos.name_service.get 'sempr'
        parts = buf.split
        if parts.length == 4
            triple = Types::sempr_rock::Triple.new
            triple.subject_ = parts[1]
            triple.predicate_ = parts[2]
            triple.object_ = parts[3]

            p sempr.removeTriple parts[0], triple
        else
            puts "Error: Need 4 inputs: entity id and triple to remove"
        end
    end
end
