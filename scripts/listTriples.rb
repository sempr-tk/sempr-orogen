require 'orocos'
require 'readline'

include Orocos
Orocos.initialize

Orocos.run do
    sempr = Orocos.name_service.get 'sempr'


    while buf = Readline.readline("(subject predicate value)> ", true)
        sempr = Orocos.name_service.get 'sempr'
        parts = buf.split
        l = sempr.listTriples parts[0], parts[1], parts[2]
        for e in l
          puts "#{e.subject_} #{e.predicate_} #{e.object_}"
        end
    end
end
