for i in {0..7}; 
do
    mv ./deploy/hotstuff-sec${i}.conf ../hotstuff-sec${i}.conf
done

mv ./deploy/hotstuff.gen.conf ../hotstuff.conf

echo "OK"