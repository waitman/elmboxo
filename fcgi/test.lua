require('mongo')

function main()

	local db = assert(mongo.Connection.New())
	assert(db:connect('localhost'))

	local q = assert(db:query('mail.keysrc', {}))
	local out = ""

	for result in q:results() do
		out = out..result.UUID.."<br>"
	end
	return (out)
end

