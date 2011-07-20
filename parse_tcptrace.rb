#!/usr/bin/ruby
require 'time'

def assert(value, str)
  if !value
    puts "ASSERT Fail: #{str}"
    exit
  end
end

if (ARGV.size == 0 or ARGV[0].include?("-h"))
  print "Usage: #{$0} <file to parse> -t <name of outputs at top to print>"
  print " -d <name of directional outputs to print>"
  puts ""
  print "  A \"*\" means that it is a value that couldn't be found for that "
  puts "connection."
  exit
end

inFilename = ARGV[0]

print "HOST_A HOST_B"
i = 1
state = 0
aryTop = Array.new
aryDir = Array.new
hshTop = Hash.new
hshDir = Hash.new
while i < ARGV.size
  if ARGV[i].include?("-t")
    state = 1
    i+=1
  elsif ARGV[i].include?("-d")
    state = 2
    i+=1
  end

  ary = ARGV[i].split
  str = ary[0].upcase
  (ary.size-1).times{|j|
    str += "_"
    str += ary[j+1].upcase
  }

  if state == 1
    print " #{str}"
    hshTop[ARGV[i]] = str
    aryTop.push(ARGV[i])
  elsif state == 2
    print " #{str}_A"
    print " #{str}_B"
    hshDir[ARGV[i]] = str
    aryDir.push(ARGV[i])
  end

  i+=1

end
puts ""

bSearching = true
bHostA = true
hshOutLine = Hash.new("*")
state = 0
input = File.open(inFilename)
input.each{|line|
  if bSearching
    bSearching = !line.include?("host")
  end

  if !bSearching and line.include?("=")
    print "#{hshOutLine["hostA"]}"
    print " #{hshOutLine["hostB"]}"
    aryTop.each{|t| print " #{hshOutLine[t]}"}
    aryDir.each{|d|
      da = d+"A"
      print " #{hshOutLine[da]}"
      db = d+"B"
      print " #{hshOutLine[db]}"
    }
    puts ""

    bSearching = line.include?("=")
    bHostA = true
    hshOutLine = Hash.new("*")
    state = 0
  end

  if !bSearching
    if state == 0 and !line.include?("->")
      ary = line.split
      if line.include?("host")
        key = "host"
        key += bHostA ? "A" : "B"
        hshOutLine[key] = ary[2]
        bHostA = false
      else
        aryTop.each{|top|
          if line.include?(top)
            if line.include?("first") or line.include?("last")
              ary = line.partition(":")
              t = Time.parse(ary[2])
              hshOutLine[top] = t.to_f
            elsif line.include?("elapsed")
              ary = line.partition(":")
              ary = ary[2].split(":")
              assert(ary.size == 3,
                     "The elapsed time is in more than 3 parts: #{ary}")
              time = ary[0].to_i*(60*60.0)
              time += ary[1].to_i*(60.0)
              time += ary[2].to_f
              hshOutLine[top] = time
            else
              i = 0
              while i < ary.size
                if ary[i].include?(":")
                  hshOutLine[top] = ary[i+1]
                  break
                end
                i+=1
              end
            end
            break
          end
        }
      end
    elsif line.include?("->")
      state += 1
    elsif state == 1
      aryDir.each{|dir|
        if line.include?(dir)
          ary1 = line.partition(":")
          ary2 = ary1[2].partition(":")
          ary3 = ary2[0].split
          ary4 = ary2[2].split
          hshOutLine["#{dir}A"] = ary3[0]
          hshOutLine["#{dir}B"] = ary4[0]
          break
        end
        }
    end

  end
}

exit
