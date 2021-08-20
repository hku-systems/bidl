# Usage: gen_log.sh $1:ENDORSE_ENABLE $2:ORDERING_ENABLE $3:E2E_VALIDATION_ENABLE $4:FULL_VALIDATION_ENABLE

file=log.json
echo "{
	Endorsement: $1,
	Ordering: $2,
	Validation: $3,
	FullCommit: $4
}" > $file
