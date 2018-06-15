require 'orocos'
require 'readline'

include Orocos
Orocos.initialize

Orocos.run do
    sempr = Orocos.name_service.get 'sempr'


    while buf = Readline.readline("> ", true)
        for result in sempr.answerQuery(buf).results
            for pair in result.pairs
                print pair.key, ":", pair.value, " "
            end
            puts ""
        end
    end
end
