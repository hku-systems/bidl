for i in {0..15}; 
do
    mv ./hotstuff.gen-sec${i}.conf ../../hotstuff-sec${i}.conf
done

mv ./hotstuff.gen.conf ../../hotstuff.conf

echo "OK"