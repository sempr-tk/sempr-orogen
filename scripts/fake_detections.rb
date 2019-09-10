require 'orocos'
require 'readline'

include Orocos
Orocos.initialize

Orocos.run do
    sempr = Orocos.name_service.get 'sempr'
    writer = sempr.detectionArray.writer
    sample = writer.new_sample

    detection = Types::mars::Detection3D.new
    detection.results[0].score = 1
    detection.results[0].pose.pose.orientation.re = 1
    detection.results[0].type = "035_power_drill"

    pt = Types::base::Vector3d.new 0, 0, 1
    detection.source_cloud.points = [pt]
    detection.source_cloud.width = 1

    sample.detections = [detection]

    position = detection.results[0].pose.pose.position
    position.x = 0
    position.y = 0
    position.z = 1

    puts sample.inspect
    writer.write sample


    for iz in 0..10 do
      z = 0.5 * iz

      y = 0
      for ix in 0..50 do
        x = -5 + ix * (5-(-5))/50.0
        puts "(x, y, z): #{x} #{y} #{z}"
  
        position = detection.results[0].pose.pose.position # necessary. don't ask me why.
        position.x = x
        position.y = y
        position.z = z
  
        # cloud must be in the same coordinate system as the pose of the detection
        # (hence x y z again, not 0 0 0) -- there is not direct correspondence to the
        # object pose, as there could be multiple hypothesese for the same cloud
        pt = Types::base::Vector3d.new x, y, z
        detection.source_cloud.points = [pt]
        detection.source_cloud.width = 1
  
        writer.write sample
  
        sleep 0.3
      end

      x = 0
      for iy in 0..50 do
        y = -5 + iy * (5-(-5))/50.0
        puts "(x, y, z): #{x} #{y} #{z}"
  
        position = detection.results[0].pose.pose.position # necessary. don't ask me why.
        position.x = x
        position.y = y
        position.z = z
  
        # cloud must be in the same coordinate system as the pose of the detection
        # (hence x y z again, not 0 0 0) -- there is not direct correspondence to the
        # object pose, as there could be multiple hypothesese for the same cloud
        pt = Types::base::Vector3d.new x, y, z
        detection.source_cloud.points = [pt]
        detection.source_cloud.width = 1
  
        writer.write sample
  
        sleep 0.3
      end
    end


#    r = 0.1
#    t = 0.0
#    dr = 0.5
#    z = 0.5
#    while true
#      t = t + 0.2
#      if t.round(1) % 6 == 0 then r = r + dr end
#      if r > 3 then 
#        r = 0
#        z = z + 0.5
#      end
#
#      x = Math.sin(t) * r
#      y = Math.cos(t) * r
#
#      puts "(t, x, y, z): #{t} #{x} #{y} #{z}"
#
#      position = detection.results[0].pose.pose.position # necessary. don't ask me why.
#      position.x = x
#      position.y = y
#      position.z = z
#
#      # cloud must be in the same coordinate system as the pose of the detection
#      # (hence x y z again, not 0 0 0) -- there is not direct correspondence to the
#      # object pose, as there could be multiple hypothesese for the same cloud
#      pt = Types::base::Vector3d.new x, y, z
#      detection.source_cloud.points = [pt]
#      detection.source_cloud.width = 1
#
#      writer.write sample
#
#      sleep 0.2
#    end
end
