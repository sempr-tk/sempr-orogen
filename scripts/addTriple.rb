require 'orocos'
require 'readline'
# require 'sempr-rock/typelib/ObjectAssertion'

include Orocos
Orocos.initialize

Orocos.run do
    sempr = Orocos.name_service.get 'sempr'


    while buf = Readline.readline("(subject predicate value)> ", true)
        sempr = Orocos.name_service.get 'sempr'

        parts = buf.split
        triple = Types::sempr_rock::Triple.new
        triple.subject_ = parts[0]
        triple.predicate_ = parts[1]
        triple.object_ = parts[2]

        p sempr.addTriple triple
    end
end
