const { describe } = require("riteway")
const { eos, names, getTableRows, isLocal, initContracts, sleep, asset, getBalanceFloat } = require("../scripts/helper")

const { firstuser, seconduser, thirduser, fourthuser, token, tokensmaster } = names


describe('Master token list', async assert => {

  if (!isLocal()) {
    console.log("only run unit tests on local - don't reset accounts on mainnet or testnet")
    return
  }

  console.log('installed at '+tokensmaster)
  const contract = await eos.contract(tokensmaster)
  const thetoken = await eos.contract(token)

  console.log('--Normal operations--')
  console.log('reset')
  await contract.reset({ authorization: `${tokensmaster}@active` })

  //return;

  console.log('init')


  await contract.init('Telos', seconduser, true, { authorization: `${tokensmaster}@active` })

  const getConfigTable = async () => {
    return await eos.getTableRows({
      code: tokensmaster,
      scope: tokensmaster,
      table: 'config',
      json: true,
    })
  }

  function dropTime(object) { object.init_time = ""; return object };
 
  assert({
    given: 'initializing contract',
    should: 'create config table',
    actual: dropTime( (await getConfigTable())['rows'][0] ),
    expected: { chain: "Telos", manager: "seedsuserbbb",
                  verify: 1, init_time: "" }
  })


  console.log('submit token')

  const testToken = token
  const testSymbol = 'SEEDS'

  await contract.submittoken(thirduser, 'Telos', testToken, testSymbol,
    '{"name": "Seeds token", "logo": "somelogo", "precision": "6", "baltitle": "Wallet balance", '+
     '"baltitle_es": "saldo de la billetera", "backgdimage": "someimg"}',
    { authorization: `${thirduser}@active` })

  const getTokenTable = async () => {
    return await eos.getTableRows({
      code: tokensmaster,
      scope: tokensmaster,
      table: 'tokens',
      json: true,
    })
  }

  assert({
    given: 'submitting token',
    should: 'create token entry',
    actual: (await getTokenTable())['rows'],
    expected: [ {id:0, submitter:"seedsuserccc",chainName:"Telos",contract:"token.seeds",symbolcode:"SEEDS",
                 json:"{\"name\": \"Seeds token\", \"logo\": \"somelogo\", \"precision\": \"6\", \"baltitle\": \"Wallet balance\", \"baltitle_es\": \"saldo de la billetera\", \"backgdimage\": \"someimg\"}"} ]
  })

  console.log('accept token')

  await contract.accepttoken(0, testSymbol, 'usecase1', true,
    { authorization: `${tokensmaster}@active` })

  const getUsecaseTable = async () => {
    return await eos.getTableRows({
      code: tokensmaster,
      scope: tokensmaster,
      table: 'usecases',
      json: true,
    })
  }

  const getAcceptanceTable = async (usecase) => {
    return await eos.getTableRows({
      code: tokensmaster,
      scope: usecase,
      table: 'acceptances',
      json: true,
    })
  }


  assert({
    given: 'accepting token',
    should: 'accept token entry',
    actual: [(await getUsecaseTable())['rows'], (await getAcceptanceTable('usecase1'))['rows'] ],
    expected: [ [{"usecase":"usecase1"} ], [{"token_id":0}] ]
  })

  console.log('add second token')
  const secondSymbol = 'TESTS'

  await contract.submittoken(fourthuser, 'Telos', testToken, secondSymbol,
    '{"name": "Test token", "logo": "somelogo", "precision": "6", "baltitle": "Wallet balance", '+
     '"baltitle_es": "saldo de la billetera", "backgdimage": "someimg"}',
    { authorization: `${fourthuser}@active` })
  await contract.accepttoken(1, secondSymbol, 'usecase1', true,
    { authorization: `${tokensmaster}@active` })

  assert({
    given: 'add token',
    should: 'approve token entry',
    actual: (await getTokenTable())['rows'],
    expected: [ {id:0, submitter:"seedsuserccc",chainName:"Telos",contract:"token.seeds",symbolcode:"SEEDS",
                 json:"{\"name\": \"Seeds token\", \"logo\": \"somelogo\", \"precision\": \"6\", \"baltitle\": \"Wallet balance\", \"baltitle_es\": \"saldo de la billetera\", \"backgdimage\": \"someimg\"}"},
                {id:1,submitter:"seedsuserxxx",chainName:"Telos",contract:"token.seeds",symbolcode:"TESTS",
                 json:"{\"name\": \"Test token\", \"logo\": \"somelogo\", \"precision\": \"6\", \"baltitle\": \"Wallet balance\", \"baltitle_es\": \"saldo de la billetera\", \"backgdimage\": \"someimg\"}"} ]
  })

  console.log('unaccept token')

  await contract.accepttoken(0, testSymbol, 'usecase1', false,
    { authorization: `${tokensmaster}@active` })

  assert({
    given: 'unaccept token',
    should: 'remove token acceptance',
    actual: (await getAcceptanceTable('usecase1'))['rows'],
    expected: [ {"token_id":1} ]
  })

  console.log('token delete')

  await contract.deletetoken(0, testSymbol, { authorization: `${seconduser}@active` })

  assert({
    given: 'deleting token',

    should: 'delete token',
    actual: [(await getUsecaseTable())['rows'], (await getTokenTable('usecase1'))['rows']],
    expected: [
       [ {"usecase":"usecase1"} ],
       [ {id:1,submitter:"seedsuserxxx",chainName:"Telos",contract:"token.seeds",symbolcode:"TESTS",
                 json:"{\"name\": \"Test token\", \"logo\": \"somelogo\", \"precision\": \"6\", \"baltitle\": \"Wallet balance\", \"baltitle_es\": \"saldo de la billetera\", \"backgdimage\": \"someimg\"}"}]
      ]
  })

  console.log('Failing operations')

  console.log('TBD')


})