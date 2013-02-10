require('mongo')
dofile('/www/bin/static/func.lua')

function main()

	local qs = vars(arg)
        local db = assert(mongo.Connection.New())
        assert(db:connect('localhost'))

	if (qs['page']~=nil) then
		page = tonumber(qs['page'])
	else
		page = 0
	end

        local q = assert(db:query('mail.mbox',{},100,page))
        local out = '<table><tr><th>Subject</th>'..
			'<th>From</th><th>Date</th><th>To</th></tr>'

        for msg in q:results() do
                local hdr = header(msg['headers'])
		if hdr then
			out = out .. '<tr>'
			if hdr['Subject'] then
				out = out .. '<td>'..hdr['Subject']..'</td>'
			else 
				out = out .. '<td> </td>'
			end
			if hdr['From'] then
				out = out .. '<td>'..hdr['From']..'</td>'
			else
				out = out .. '<td> </td>'
			end
			if hdr['Date'] then
				out = out .. '<td nowrap="nowrap">'..hdr['Date']..'</td>'
			else 
				out = out .. '<td> </td>'
			end
			if hdr['To'] then
				out = out .. '<td>'..hdr['To']..'</td>'
			else
				out = out .. '<td> </td>'
			end
			out = out .. '</tr>'
		end
        end
	out = out .. '</table>'

	return (out)
end
