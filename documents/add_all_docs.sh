doc_id=0
> wikitext_metadata_full.txt
for file in wikitext/*.txt; do
    if [ -f "$file" ]; then
        echo "$doc_id $file" >> wikitext_metadata_full.txt
        ((doc_id++))
    fi
done
echo "Created metadata with $doc_id documents"
