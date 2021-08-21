package chaincode

import (
	"encoding/json"
	"fmt"
	"time"

	// "github.com/hyperledger/fabric/common/flogging"
	pb "github.com/hyperledger/fabric/protos/peer"

	log "github.com/sirupsen/logrus"


	"github.com/spf13/cobra"
)

// var logger = flogging.MustGetLogger("tps.go")


var chaincodeTpsCmd *cobra.Command


func tpsCmd(cf *ChaincodeCmdFactory) *cobra.Command {
	chaincodeTpsCmd = &cobra.Command{
		Use:       "tps",
		Short:     fmt.Sprintf("Test the tps of the specified %s.", chainFuncName),
		Long:      fmt.Sprintf("Test the tps of the specified %s. It will try to commit the endorsed transaction to the network.", chainFuncName),
		ValidArgs: []string{"1"},
		RunE: func(cmd *cobra.Command, args []string) error {
			return chaincodeTps(cmd, cf)
		},
	}
	flagList := []string{
		"tpsConfig",
	}
	attachFlags(chaincodeTpsCmd, flagList)

	return chaincodeTpsCmd
}

func chaincodeTps(cmd *cobra.Command, cf *ChaincodeCmdFactory) error {
	logger := log.New()
	logger.SetLevel(log.WarnLevel)

	cmd.SilenceUsage = true
	fmt.Printf("jyp: readFromConfigFile=%s\n", tpsConfigFile)

	config := LoadConfig(tpsConfigFile)
	// fmt.Println(config.Endorsers)

	N := config.TXs
	crypto := config.LoadCrypto()
	assember := &Assembler{Signer: crypto, idx: 0}

	raw := make(chan *Elements, 10000)
	signed := make([]chan *Elements, len(config.Endorsers))
	processed := make(chan *Elements, 1000)
	envs := make(chan *Elements, 1000)
	done := make(chan struct{})

	for i := 0; i < len(config.Endorsers); i++ {
		signed[i] = make(chan *Elements, 1000)
	}

	for i := 0; i < 5; i++ {
		go assember.StartSigner(raw, signed, done)
		go assember.StartIntegrator(processed, envs, done)
	}

	proposor := CreateProposers(config.NumOfConn, config.ClientPerConn, config.Endorsers, logger)
	proposor.Start(signed, processed, done, config)

	broadcaster := CreateBroadcasters(config.NumOfConn, config.Orderer, logger)
	broadcaster.Start(envs, done)

	observer := CreateObserver(config.Channel, config.Committer, crypto, logger)

	// generate txns or readFromFile(TODO)
	chaincodeCtorJSONs := GenerateWorkload(N)

	start := time.Now()
	go observer.Start(N, start)

	for i := 0; i < N; i++ {
		chaincodeCtorJSON := chaincodeCtorJSONs[i]
		input := &pb.ChaincodeInput{}
		if err := json.Unmarshal([]byte(chaincodeCtorJSON), &input); err != nil {
			return err
		}
		prop := CreateProposal(
			crypto,
			config.Channel,
			config.Chaincode,
			config.Version,
			input,
		)
		raw <- &Elements{Proposal: prop}
	}

	observer.Wait()
	duration := time.Since(start)
	close(done)
	logger.Infof("Completed processing transactions.")
	fmt.Printf("tx: %d, duration: %+v, tps: %f\n", N, duration, float64(N)/duration.Seconds())
	return nil
}
