
--src:http://lua-users.org/wiki/StringRecipes
function url_decode(str)
  str = string.gsub (str, "+", " ")
  str = string.gsub (str, "%%(%x%x)",
      function(h) return string.char(tonumber(h,16)) end)
  str = string.gsub (str, "\r\n", "\n")
  return str
end

--src:http://lua-users.org/wiki/SplitJoin
function explode(d,p)
  local t, ll
  t={}
  ll=0
  if(#p == 1) then return {p} end
    while true do
      l=string.find(p,d,ll,true) 
      if l~=nil then 
        table.insert(t, string.sub(p,ll,l-1)) 
        ll=l+1 
      else
        table.insert(t, string.sub(p,ll)) 
        break 
      end
    end
  return t
end

function vars(arg)
	local qs = {}
	for i=1,table.getn(arg) do
	        if arg[i] ~= nil then
	                for x,y in pairs(explode('&',arg[i])) do
	                        j = explode('=',y)
	                        if (j[2] ~= nil) then
	                                qs[j[1]]=url_decode(j[2])
	                        end
	                end     
	        end     
	end
	return qs
end

function htmlchar(v)
	local x = string.gsub(v,'<','&lt;')
	x = string.gsub(x,'>','&gt;')
	x = string.gsub(x,'"','&quot;')
	x = string.gsub(x,"'",'&apos;')
	return (x)
end

function header(o)
	hdr = {}
	if type(o) == 'table' then
		for k,v in pairs(o) do
			for l,m in pairs(v) do
				if l ~= 'ph' then
					hdr[l]=htmlchar(m)
				end
			end
		end
		return hdr
	end
end

