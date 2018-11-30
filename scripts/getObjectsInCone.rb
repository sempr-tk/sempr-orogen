require 'orocos'
require 'readline'

if __FILE__ == $0

  Orocos.initialize
  #Orocos.registry.get("/base/Pose_m")

  a = 0.707

  pose = { "position" => {"data"=> [-15.0,0.0,1.05]}, "orientation"=>{"im"=> [0,0,0], "re"=>1}}

  length = 10.0
  angle = 45.0   

  Orocos.run do
    sempr = Orocos.name_service.get 'sempr'

    list = sempr.getObjectsInCone(pose, length, angle, "http://trans.fit/Screwdriver-")
    p list
  end
end
